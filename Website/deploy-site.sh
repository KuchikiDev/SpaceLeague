#!/usr/bin/env bash
set -euo pipefail
archive=${1:?Archive required}
expected_sha=${2:?SHA-256 required}
release=${3:?Release required}
config=${4:?Nginx config required}
[[ "$release" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$ ]] || exit 2
[[ "$expected_sha" =~ ^[a-f0-9]{64}$ ]] || exit 2
[[ -f "$archive" && -f "$config" ]] || exit 2
printf '%s  %s\n' "$expected_sha" "$archive" | sha256sum --check --status
root=/opt/ora/site
target="$root/releases/$release"
[[ ! -e "$target" ]] || { echo 'Release already exists'; exit 2; }
if tar -tzf "$archive" | grep -Eq '(^/|(^|/)\.\.(/|$))'; then
    echo 'Unsafe archive path'; exit 2
fi
mkdir -p "$target" "$root/backups"
tar -xzf "$archive" --no-same-owner --no-same-permissions -C "$target"
test -s "$target/index.html"
test -s "$target/app.js"
test -s "$target/style.css"
test -s "$target/character-model.js"
test -s "$target/character-three.js"
test -s "$target/vendor/THREE-LICENSE.txt"
find "$target" -type d -exec chmod 755 {} +
find "$target" -type f -exec chmod 644 {} +
previous=""
if [[ -L "$root/current" ]]; then previous=$(readlink -e "$root/current" || true); fi
if [[ -n "$previous" && "$previous" != "$root"/releases/* ]]; then echo 'Unexpected current target'; exit 2; fi
nginx_config=/etc/nginx/sites-available/ora-poc.conf
nginx_enabled=/etc/nginx/sites-enabled/ora-poc.conf
had_config=false
had_enabled=false
if [[ -e "$nginx_enabled" || -L "$nginx_enabled" ]]; then had_enabled=true; fi
if [[ -f "$nginx_config" ]]; then
    cp -a "$nginx_config" "$root/backups/nginx-$release.conf"
    had_config=true
fi
rollback() {
    if [[ -n "$previous" && -d "$previous" ]]; then
        ln -sfn "$previous" "$root/current-rollback"
        mv -Tf "$root/current-rollback" "$root/current"
    fi
    if $had_config; then
        cp -a "$root/backups/nginx-$release.conf" "$nginx_config"
    fi
    if ! $had_enabled; then rm -f "$nginx_enabled"; fi
    nginx -t && systemctl reload nginx
    echo 'Deployment rolled back' >&2
}
trap rollback ERR
ln -s "$target" "$root/current-next"
mv -Tf "$root/current-next" "$root/current"
install -m 644 "$config" "$nginx_config"
ln -sfn "$nginx_config" "$nginx_enabled"
nginx -t
systemctl reload nginx
# Origin certificate is trusted by Cloudflare; this loopback check skips public CA validation only.
healthy=false
for attempt in 1 2 3 4 5; do
    if curl --fail --silent --insecure --resolve poc.oragame.eu:443:127.0.0.1 https://poc.oragame.eu/ -o "$root/health-response.html" && grep -q 'Par-delà les îlots' "$root/health-response.html"; then
        healthy=true
        break
    fi
    sleep 1
done
$healthy
mkdir -p /srv/services/ora
if [[ ! -e /srv/services/ora/site && ! -L /srv/services/ora/site ]]; then ln -s "$root" /srv/services/ora/site; fi
printf '{"release":"%s","archive_sha256":"%s","host":"poc.oragame.eu"}\n' "$release" "$expected_sha" > "$root/release-manifest.json"
trap - ERR
echo "ORA site deployed: $release"
