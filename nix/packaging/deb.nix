# nix/packaging/deb.nix
#
# Debian package generation for XDP2.
# Creates staging directory and .deb package using FPM.
#
# Phase 1: x86_64 only
# See: documentation/nix/microvm-implementation-phase1.md
#
{ pkgs, lib, xdp2 }:

let
  metadata = import ./metadata.nix;

  # Determine Debian architecture from system
  debArch = metadata.debArchMap.${pkgs.stdenv.hostPlatform.system} or "amd64";

  # Create staging directory with FHS layout
  # This mirrors the final installed structure
  staging = pkgs.runCommand "xdp2-staging-${debArch}" {} ''
    echo "Creating staging directory for ${debArch}..."

    # Create FHS directory structure
    mkdir -p $out/usr/bin
    mkdir -p $out/usr/lib
    mkdir -p $out/usr/include/xdp2
    mkdir -p $out/usr/share/xdp2
    mkdir -p $out/usr/share/doc/xdp2

    # Copy binaries
    echo "Copying binaries..."
    if [ -f ${xdp2}/bin/xdp2-compiler ]; then
      cp -v ${xdp2}/bin/xdp2-compiler $out/usr/bin/
    else
      echo "WARNING: xdp2-compiler not found"
    fi

    if [ -f ${xdp2}/bin/cppfront-compiler ]; then
      cp -v ${xdp2}/bin/cppfront-compiler $out/usr/bin/
    else
      echo "WARNING: cppfront-compiler not found"
    fi

    # Copy libraries (shared and static)
    echo "Copying libraries..."
    if [ -d ${xdp2}/lib ]; then
      for lib in ${xdp2}/lib/*.so ${xdp2}/lib/*.a; do
        if [ -f "$lib" ]; then
          cp -v "$lib" $out/usr/lib/
        fi
      done
    else
      echo "WARNING: No lib directory in xdp2"
    fi

    # Copy headers
    echo "Copying headers..."
    if [ -d ${xdp2}/include ]; then
      cp -rv ${xdp2}/include/* $out/usr/include/xdp2/
    else
      echo "WARNING: No include directory in xdp2"
    fi

    # Copy templates and shared data
    echo "Copying shared data..."
    if [ -d ${xdp2}/share/xdp2 ]; then
      cp -rv ${xdp2}/share/xdp2/* $out/usr/share/xdp2/
    fi

    # Create basic documentation
    cat > $out/usr/share/doc/xdp2/README << 'EOF'
    ${metadata.longDescription}

    Homepage: ${metadata.homepage}
    License: ${metadata.license}
    EOF

    # Create copyright file (required for Debian packages)
    cat > $out/usr/share/doc/xdp2/copyright << 'EOF'
    Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
    Upstream-Name: ${metadata.name}
    Upstream-Contact: ${metadata.maintainer}
    Source: ${metadata.homepage}

    Files: *
    Copyright: 2024 XDP2 Authors
    License: ${metadata.license}
    EOF

    echo "Staging complete. Contents:"
    find $out -type f | head -20 || true
  '';

  # Format long description for Debian control file
  # Each continuation line must start with exactly one space
  formattedLongDesc = builtins.replaceStrings ["\n"] ["\n "] metadata.longDescription;

  # Control file content (generated at Nix evaluation time)
  controlFile = pkgs.writeText "control" ''
    Package: ${metadata.name}
    Version: ${metadata.version}
    Architecture: ${debArch}
    Maintainer: ${metadata.maintainer}
    Description: ${metadata.description}
     ${formattedLongDesc}
    Homepage: ${metadata.homepage}
    Depends: ${lib.concatStringsSep ", " metadata.debDepends}
    Section: devel
    Priority: optional
  '';

  # Generate .deb package using dpkg-deb (native approach)
  # FPM fails in Nix sandbox due to lchown permission issues
  deb = pkgs.runCommand "xdp2-${metadata.version}-${debArch}-deb" {
    nativeBuildInputs = [ pkgs.dpkg ];
  } ''
    mkdir -p $out
    mkdir -p pkg

    echo "Generating .deb package using dpkg-deb..."
    echo "  Name: ${metadata.name}"
    echo "  Version: ${metadata.version}"
    echo "  Architecture: ${debArch}"

    # Copy staging contents to working directory
    cp -r ${staging}/* pkg/

    # Create DEBIAN control directory
    mkdir -p pkg/DEBIAN

    # Copy pre-generated control file
    cp ${controlFile} pkg/DEBIAN/control

    # Create md5sums file
    (cd pkg && find usr -type f -exec md5sum {} \;) > pkg/DEBIAN/md5sums

    # Build the .deb package
    dpkg-deb --build --root-owner-group pkg $out/${metadata.name}_${metadata.version}_${debArch}.deb

    echo ""
    echo "Package created:"
    ls -la $out/

    echo ""
    echo "Package info:"
    dpkg-deb --info $out/*.deb

    echo ""
    echo "Package contents (first 30 files):"
    dpkg-deb --contents $out/*.deb | head -30 || true
  '';

in {
  inherit staging deb metadata;

  # Expose architecture for debugging
  arch = debArch;
}
