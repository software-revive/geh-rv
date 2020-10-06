Geh is a simple command line image viewer written in C/Gtk+ with
various nice features. It currently supports the following modes:

 * Slide-show mode displaying multiple images in a row either
   controlled by an time interval and/or mousebutton clicks.
 * Thumbnail mode creating small thumbnail images and caching them
   in `~/.thumbnails` according to freedesktop.org's thumbnail
   specification.

This is a fork being developed as part of the Software Revive initiative.

For more information check out the geh project page at:
https://github.com/software-revive/geh-rv

The original project URL is:
https://projects.pekdon.net/projects/geh

## What's new

See [NEWS](NEWS) for details about changes between releases.

We don't maintain the ChangeLog file (in the GNU compatible format), since
`git log` does just the same job.

## Installation

### Arch Linux

 - [GTK+ 2 build](https://aur.archlinux.org/packages/geh-gtk2-git)
 - [GTK 3 build](https://aur.archlinux.org/packages/geh-git)

### Manual installation

If you build the program from the git source tree, run `./autogen.sh` first to
generate the `configure` script. If you build the program from a release
tarball, the `configure` script is already present.

Geh can be built with gtk2 or gtk3.

To build the gtk2 version, run:

    ./configure --prefix=<your installation path> --enable-gtk2
    make && make install

To build the gtk3 version, run:

    ./configure --prefix=<your installation path> --disable-gtk2
    make && make install

Geh compiles to a single binary. No additional dependencies, just the standard
gtk-related shared libraries (pixbuf, gtk2/3, pango, cairo etc).

## Usage

See the output of `geh --help` for detals.

## Keybindings

 * `f`, zoom to fit.
 * `F`, full mode showing only the current picture.
 * `s`, slide mode showing a large picture and one row of thumbnails.
 * `S`, open save picture dialog.
 * `R`, open rename picture dialog.
 * `t` `T`, thumbnail mode showing only thumbnails.
 * `q` `Q`, quit.
 * `n` `N`, show/select next image.
 * `p` `P`, show/select previous image.
 * `+`, zoom in current image.
 * `-`, zoom out current image.
 * `0`, zoom current image to original size.
 * `F11`, toggle fullscreen mode.

