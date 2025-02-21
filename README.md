# man-opoly
man-opoly is a GTK4-based application that provides a fast, user-friendly interface for searching and viewing Linux manual pages. Unlike the traditional `man` command, this app makes it easier to find and navigate through manual pages.

# what are man pages?

man pages (short for manual pages) are the built-in documentation for Linux commands, system calls, standard C library, and so many more. there are currently 7 sections, with each dedicated to a different category of documentation. For more information, you can refer to [Wikipedia](https://en.wikipedia.org/wiki/Man_page)

### why i built this

using the `man` command is great, but:

- finding the right page is sometimes very hard. you don't always know the exact name
- jumping between related pages is frustrating. you need to go back and forth between documentations since what you're doing isn't always just a simple function in C.
- terminal/vim navigation can be a hassle if you're doing it for a long time.

## Features

- Fuzzy search - An extremely fast keyword matching algorithm that helps you find the right man page
- Tabbed browsing - Open multiple pages at the same time 
- Documentation - The home screen greets you with a brief introduction + which sections you might be looking for
- Fast and lightweight - Since it is built completely in C, it is extremely lightweight and quick. Great for lower spec-ed machines.

# Installation
1. Install Dependencies
Make sure you have GTK4 and other required libraries installed. 

- Debian/Ubuntu
```bash
sudo apt install libgtk-4-dev make gcc
```

- Arch Linux
```bash
sudo pacman -S gtk4 make gcc
```

- dnf/Fedora
```bash
sudo dnf install gtk4-devel make gcc
```


2. Clone the repository

```bash
git clone https://github.com/kysakhin/man-opoly.git
cd man-opoly
```


3. Build the project
```bash
make
```

4. Run the application
```bash
./app
```

# License
This project is **open-source** under the MIT License

## Future plans

- Improve formatting with more syntax highlighting
- Implement search-as-you-type functionality
- Explore future WebKitGTK improvements for better rendering

### Roadblocks Faced

During development, I initally thought of rendering the manual pages as HTML and displaying them in a WebView. However, GTK4's WebKitGTK had some differences from GTK3, and there was no support as of yet for GTK4.

This could allow for clickable links to other man pages, making user experience extremely fruitful.

