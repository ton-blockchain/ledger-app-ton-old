# Ledger TON Coin app

## Overview
TON Coin Ledger app for Ledger Nano S/X

## Building and installing
To build and install the app on your Ledger Nano S you must set up the Ledger Nano S build environments. Please follow the Getting Started instructions at [here](https://ledger.readthedocs.io/en/latest/userspace/getting_started.html).

If you don't want to setup a global environnment, you can also setup one just for this app by sourcing `prepare-devenv.sh` with the right target (`s` or `x`).

Install prerequisite and switch to a Nano S dev-env:

```bash
sudo apt install gcc make gcc-multilib g++-multilib libncurses5
sudo apt install python3-venv python3-dev libudev-dev libusb-1.0-0-dev

# (s or x, depending on your device)
source prepare-devenv.sh s 
```

To fix problems connecting to Ledger follow this [guide](https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues)

Compile and load the app onto the device:
```bash
make load
```

Refresh the repo (required after Makefile edits):
```bash
make clean
```

Remove the app from the device:
```bash
make delete
```

Enable debug mode:
```bash
make clean DEBUG=1 load
```

## Documentation
This follows the specification available in the [`toncoinapp.asc`](https://github.com/play-ton/ledger-app-toncoin/blob/master/doc/toncoinapp.asc)
