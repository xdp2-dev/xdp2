# nix/shell-functions/ascii-art.nix
#
# ASCII art logo display for XDP2 development shell
#
# Uses jp2a to convert the XDP2 logo to colored ASCII art.
# Falls back to a simple text banner if jp2a is not available.
#
# Usage in devshell.nix:
#   asciiArt = import ./shell-functions/ascii-art.nix { };
#

{ }:

''
  if command -v jp2a >/dev/null 2>&1 && [ -f "./documentation/images/xdp2-big.png" ]; then
    echo "$(jp2a --colors ./documentation/images/xdp2-big.png)"
    echo ""
  else
    echo "🚀 === XDP2 Development Shell ==="
  fi
''
