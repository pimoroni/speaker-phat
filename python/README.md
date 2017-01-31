# basic python API for controlling LEDs

The present API allows controlling the Speaker pHAT using Python.

Before using it, you need to install the sn3218 module, like so:

On Raspbian:

```
sudo apt-get install python-sn3218 python3-sn3218
```

On other distributions:

```
sudo pip install sn3218
```

or

```
sudo pip3 install sn3218
```

Use this API as follows:

```python
from speaker-phat import clear, show, set_led

clear()
for x in range(10):
    set_led(x,255)
show()
```

`set_led(index, value)`

index: led index between 0 to 9
value: brightness between 0 and 255
