# loraham-pi

## TODO BEFORE RELEASE

- don't RT messages we've already RT'd

## Setup

- You'll need at least raspian buster
- Some deps require `rustc`, fetch it from rustup.rs
- create a venv: `python -m venv env`
- and use it: `source env/bin/activate`
- install python dependencies: `python -m pip install -r requirements.txt`
- `sudo raspi-config` > interfaces > I2C > enable
- `sudo raspi-config` > interfaces > SPI > enable

## Running

When run, the program simply wraps all traffic (including its own) in JSON and sends to stdout,
allowing you to pipe it to whatever you'd like.

Example:

```
$ export CALLSIGN=KD2LYD-0
$ python loragw.py
``` 

If you'd like to log, use something like `multilog` which will 
automatically rotate logs when they get to a specific size/number

```
$ python loragw.py | multilog s10000 ./logs
```

If you'd like to log *and* do any number of other things with stdout, use `tee`

```
$ python loragw.py | tee >(multilog s10000 ./logs) >(./myparser.py) >(./send_to_pubsub)
```
