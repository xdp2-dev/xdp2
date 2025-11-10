# xdp2 samples

This folder contains the samples for the xdp2 project.

## Usage

Set XDP2DIR to the install directory for XDP2 like
```
make XDP2DIR=~/xdp2/install
```

This should probably be set to the same as:
```
das@ubuntu2404-no-nix-no-version:~/xdp2/samples$ cat ~/xdp2/src/config.mk | grep INSTALLDIR
INSTALLDIR ?= /home/das/xdp2/src/../install/x86_64
```

Therefore, recommend:
```
make XDP2DIR=~/xdp2/install/x86_64/
```