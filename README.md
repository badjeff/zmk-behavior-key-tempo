# Key Tempo Behavior

This is a version of [Key Repest](https://zmk.dev/docs/behaviors/key-repeat) with on-the-fly recording feature for ZMK with at least Zephyr 3.5.

## What it does

This behavior requires a double tap to record the duration of key press/reelase states in runtime. And then, it raises last capture keycode repeatly, until the thrid tap to stop.

In other words, the mechanism is similar to tap the tempo on synthesizer.
Record the tempo, playback, then stop.

## Installation

Include this project on your ZMK's west manifest in `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: badjeff
      url-base: https://github.com/badjeff
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    # START #####
    - name: zmk-behavior-key-tempo
      remote: badjeff
      revision: main
    # END #######
  self:
    path: config
```

Now, update your `shield.keymap` adding the behaviors. You can check the `usage-pages` on [Key Repest](https://zmk.dev/docs/behaviors/key-repeat) doc page.

```keymap
/ {
        behaviors {
                 tempo: key_tempo {
                        compatible = "zmk,behavior-key-tempo";
                        #binding-cells = <0>;
                        // usage-pages = <HID_USAGE_KEY>;
                        usage-pages = <HID_USAGE_KEY HID_USAGE_CONSUMER>;
                };
        };

        keymap {
                compatible = "zmk,keymap";
                default_layer {
                        bindings = <

                              ..... .... .... &tempo .... .... .....

                        >;
                };
       };

};
```
