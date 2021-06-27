# Ledger TON app

## Overview
Ledger TON app for Ledger Nano S/X

## Building and installing

Install prerequisite and use `prepare-devenv.sh` with the right target (`s` or `x`):

```bash
sudo apt install gcc make gcc-multilib g++-multilib libncurses5
sudo apt install python3-venv python3-dev libudev-dev libusb-1.0-0-dev

# (s or x, depending on your device)
source prepare-devenv.sh s 
```

To fix problems connecting to Ledger follow this [guide](https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues) (Open the Linux dropdown list at bottom of the page).

Compile and load the app onto the device:
```bash
make load
```

Refresh the repo (required after Makefile edits):
```bash
make clean
```
> An "arm-none-eabi-gcc: Command not found" may appear - it's OK, doesn't affect anything.


Remove the app from the device:
```bash
make delete
```

Enable debug mode:
```bash
make clean DEBUG=1 load
```

## Documentation
This follows the specification available in the [`tonapp.asc`](https://github.com/newton-blockchain/ledger-app-ton/blob/main/doc/tonapp.asc)
