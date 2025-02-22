#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/fuzzy.h"

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static bool is_exact_match(const char *pattern, const char *str) {
    while (*pattern && *str) {
        if (tolower((unsigned char)*pattern) != tolower((unsigned char)*str)) {
            return false;
        }
        pattern++;
        str++;
    }
    return *pattern == '\0';
}

int compute_score(int32_t jump, bool first_char, const char *restrict match);

int fuzzy_match_recurse(const char *restrict pattern, const char *restrict str, int score, bool first_char);

int fuzzy_match(const char *pattern, const char *str) {
    const int unmatched_letter_penalty = -1;
    const int exact_match_bonus = 1000;
    const size_t slen = strlen(str);
    const size_t plen = strlen(pattern);
    int score = 100;

    if (*pattern == '\0') return score;
    if (slen < plen) return INT32_MIN;

    if (is_exact_match(pattern, str)) {
        return score + exact_match_bonus;
    }

    const char *opening_paren = strchr(str, '(');
    if (opening_paren) {
        size_t prefix_len = opening_paren - str;
        if (prefix_len == strlen(pattern) && is_exact_match(pattern, str)) {
            return score + exact_match_bonus - 50; // Slightly lower bonus for matches with suffixes
        }
    }

    score += unmatched_letter_penalty * (int)(slen - plen);
    score = fuzzy_match_recurse(pattern, str, score, true);
    return score;
}

int fuzzy_match_recurse(const char *restrict pattern, const char *restrict str, int score, bool first_char) {
    if (*pattern == '\0') return score;

    const char *match = str;
    const char search[2] = {*pattern, '\0'};
    int best_score = INT32_MIN;
    
    bool at_word_start = (str == match || !isalnum((unsigned char)*(str-1)));
    if (at_word_start) {
        score += 50;
    }

    while ((match = strcasestr(match, search)) != NULL) {
        int subscore = fuzzy_match_recurse(
            pattern + 1,
            match + 1,
            compute_score(match - str, first_char, match),
            false
        );
        best_score = MAX(best_score, subscore);
        match++;
    }

    if (best_score == INT32_MIN) return INT32_MIN;
    return (score + best_score);
}

int32_t compute_score(int32_t jump, bool first_char, const char *restrict match) {
    const int adjacency_bonus = 15;
    const int separator_bonus = 30;
    const int camel_bonus = 30;
    const int first_letter_bonus = 15;
    const int leading_letter_penalty = -5;
    const int max_leading_letter_penalty = -15;
    const int consecutive_match_bonus = 40;
    
    int32_t score = 0;

    if (!first_char && jump == 0) {
        score += adjacency_bonus + consecutive_match_bonus;
    }

    if (!first_char || jump > 0) {
        if (isupper((unsigned char)*match) && islower((unsigned char)*(match-1))) {
            score += camel_bonus;
        }
        if (isalnum((unsigned char)*match) && !isalnum((unsigned char)*(match-1))) {
            score += separator_bonus;
        }
    }

    if (first_char && jump == 0) {
        score += first_letter_bonus;
    }
    if (first_char) {
        score += MAX(leading_letter_penalty * jump, max_leading_letter_penalty);
    }

    return score;
}
