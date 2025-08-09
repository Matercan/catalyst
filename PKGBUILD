# Maintainer: Matercan<172917897+Matercan@users.noreply.github.com>
pkgname='catalyst'
pkgver=1.0.0
pkgrel=1
pkgdesc="Hyprland keybinds, the Nix way"
arch=('x86_64')
url="https://github.com/Matercan/catalyst"
license=('GPL')
depends=('hyprland>=v0.49.0' 'wayland')
makedepends=('gcc' 'git')
source=('git+https://github.com/Matercan/catalyst.git')
sha256sums=('SKIP')


build() {
    cd "$pkgname"
    g++ main.cpp -o catalyst-bin -l jsoncpp  
}

package() {
    cd "$pkgname"
    install -Dm755 catalyst-bin "$pkgdir/usr/bin/catalyst"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    touch "${XDG_CONFIG_HOME}/catalyst/config.json"
}
