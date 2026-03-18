# nix/shell-functions/configure.nix
#
# Configure shell functions for XDP2
#
# Functions:
# - smart-configure: Smart configure script with age checking
#
# Note: These functions depend on navigation functions from navigation.nix
#
# Usage in flake.nix:
#   configureFns = import ./nix/shell-functions/configure.nix { configAgeWarningDays = 14; };
#

{ configAgeWarningDays ? 14 }:

''
  # Smart configure script execution with age checking
  # This simply includes a check to see if the config.mk file exists, and
  # it generates a warning if the file is older than the threshold
  smart-configure() {
    local config_file="./src/config.mk"
    local warning_days=${toString configAgeWarningDays}

    if [ -f "$config_file" ]; then
      echo "✓ config.mk found, skipping configure step"

      # Check age of config.mk
      local file_time
      file_time=$(stat -c %Y "$config_file")
      local current_time
      current_time=$(date +%s)
      local age_days=$(( (current_time - file_time) / 86400 ))

      if [ "$age_days" -gt "$warning_days" ]; then
        echo "⚠️  WARNING: config.mk is $age_days days old (threshold: $warning_days days)"
        echo "   Consider running 'configure' manually if you've made changes to:"
        echo "   • Build configuration"
        echo "   • Compiler settings"
        echo "   • Library paths"
        echo "   • Platform-specific settings"
        echo ""
      else
        echo "✓ config.mk is up to date ($age_days days old)"
      fi
    else
      echo "config.mk not found, running configure script..."
      cd src || return 1
      rm -f config.mk
      ./configure.sh --build-opt-parser

      # Apply PATH_ARG fix for Nix environment
      if grep -q 'PATH_ARG="--with-path=' config.mk; then
        echo "Applying PATH_ARG fix for Nix environment..."
        sed -i 's|PATH_ARG="--with-path=.*"|PATH_ARG=""|' config.mk
      fi
      echo "PATH_ARG in config.mk: $(grep '^PATH_ARG=' config.mk)"

      cd .. || return 1
      echo "✓ config.mk generated successfully"
    fi
  }
''
