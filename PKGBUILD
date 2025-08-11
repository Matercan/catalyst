# Maintainer: Matercan<172917897+Matercan@users.noreply.github.com>

_commit=a274625cb24658f67129681f17dcf0dd36b85d88

pkgname='catalyst'
pkgver=1.0.5.r0.g3d0389c  # This will be updated by pkgver()
pkgrel=6
pkgdesc="Hyprland keybinds, the Nix way"
arch=('x86_64')
url="https://github.com/Matercan/catalyst"
license=('GPL')
depends=('hyprland>=v0.49.0' 'wayland')
makedepends=('gcc' 'jsoncpp' 'opencv' 'git')

# Use git source for signed commits
source=("git+${url}.git#commit=${_commit}")
sha256sums=('SKIP')

# For GPG-signed commits, specify the signing key
validpgpkeys=('49EA0D26F3450D265D030EC9FC94536D7EA85227')

pkgver() {
    cd "$pkgname"
    git describe --long --tags | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
}

prepare() {
    cd "$pkgname"
    # Verify the commit signature
    git verify-commit ${_commit} || {
        echo "ERROR: Commit ${_commit} is not properly signed!"
        exit 1
    }
}

build() {
    cd "$pkgname"
    g++ main.cpp -o catalyst-bin -ljsoncpp
}

package() {
    cd "$pkgname"
    
    install -Dm755 catalyst-bin "$pkgdir/usr/bin/catalyst"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
