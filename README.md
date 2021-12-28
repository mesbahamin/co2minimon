Put the udev rules in /etc/udev/rules.d/, then reload the rules with:

```
# udevadm control --reload-rules && udevadm trigger
```
