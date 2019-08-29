# Brief
These routine test for nimble under the rt-thread platform.
Refer to the rt-thread official website and apache nimble official website for details.

RT-THREAD:``https://github.com/RT-Thread/rt-thread``

NIMBLE:``http://mynewt.apache.org/latest/network/docs/index.html``

# Example

## 1.ble_test
These routines are used to learn the nimble protocol stack.

BLE stack runs after `ble_hr` is entered

```
msh />ble_hr
[I/nimble] GAP procedure initiated: stop advertising.
[I/nimble] GAP procedure initiated: advertise; disc_mode=2 adv_channel_map=0 own_addr_type=0
adv_filter_policy=0 adv_itvl_min=0 adv_itvl_max=0
msh />
```
