<img src="images/Hop.png" alt="Hop the Panda Logo" align="right"/>

The XDP2 TC Classifier
================

The XDP2 compiler is able to generate code that runs as a Linux kernel module.

We provide a TC classifier, the XDP2 classifier, that can be used to leverage
the XDP2 parser to do classification of packets.

The classifier is still a work-in-progress.

## Installation

   In order to install the classifier kernel module

   ```
   cd src/kernel
   make modules_install
   ```

## iproute2

  In order to use the classifier you must patch the iproute2 tree.
  The patch is located under `src/tc/tc.patch`.

  ```
     # Example workflow for patching
     git clone https://github.com/shemminger/iproute2.git
     cd iproute2
     patch -p1 < xdp2/src/tc/tc.patch
  ```

  The patch adds support for the xdp2 classifier to the `tc` command line.

  Whenever the `tc` command line is invoked with the classifier, it tries to load
  the `cls_xdp2` module.
  The same happens inside the classifier, which tries to load the
  module `xdp2_<name>`.

## Usage

   The XDP2 classifier is a TC classifier that loads a dynamic parser to do
   classification.

   ```
      # Parser xdp2_foo.ko module
      $ tc filter [...] xdp2 parser foo [...]
   ```

   So far we have not implementation any `tc` actions.
   The current implementation is a proof of concept for using a parser as a dynamic component.

## Samples

   The `samples/kmod` directory contains programs that uses the XDP2
   classifier.  Refer to these programs for building parsing modules for the
   XDP2 classifier.

   The kernel module samples calls `pr_debug()` when appropriate.
   The messages are enabled by either compiling the modules in debug mode or by using dynamic debugging.

   Read more at
   https://www.kernel.org/doc/html/latest/admin-guide/dynamic-debug-howto.html
   https://elixir.bootlin.com/linux/latest/source/include/linux/printk.h#L411

## Special macros

- `XDP2_MAKE_TC_PARSER_PROGRAM(NAME, FUNC)`

   Template to register the parser `NAME` associating the function `FUNC`.
   The parser name `NAME` is used by the tc command line to load the parsing
   module.  The function `FUNC` is the entry point to the dynamic parser called
   by the classifier.
