# Maintainer: Amin Mesbah <dev@aminmesbah.com>

# https://wiki.archlinux.org/title/PKGBUILD

pkgname=co2minimon
pkgver=0.0.1
pkgrel=1
pkgdesc='Daemon for CO2Mini sensor.'
arch=('any')
license=('Unlicense')
makedepends=('bash' 'clang')
provides=('co2minimon')
source=(build.sh
        main.c
        LICENSE
        ${pkgname}.service)
# update with updpkgsums from pacman-contrib
sha256sums=('760c19a24dce4341650857d28f3f6e6f7fe788428c9658ff9082d01c6afaec72'
            '07b2e03e16c38b2e98c0c0f293196d8a168cc5189d72fed2bf4859cc9d0e903f'
            '7e12e5df4bae12cb21581ba157ced20e1986a0508dd10d0e8a4ab9a4cf94e85c'
            '2d6b333afdf16af84232032040703c054b5529a79d4bea4e65831fd50b05635a')


build() {
    /usr/bin/env bash build.sh || exit 1
}

package() {
    install -D --mode=644 ${pkgname}.service "${pkgdir}/usr/lib/systemd/user/${pkgname}.service" || exit 1
    install -D --mode=755 out/release/co2minimon "${pkgdir}/usr/bin/co2minimon" || exit 1
    install -D --mode=644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE" || exit 1
}
