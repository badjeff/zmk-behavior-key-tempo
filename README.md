# Key Tempo Behavior

This is a version of [Key Repeat](https://zmk.dev/docs/behaviors/key-repeat) with on-the-fly recording feature for ZMK with at least Zephyr 3.5.

## What it does

This behavior requires a double tap to record the duration of key press/release states in runtime. And then, it raises last capture keycode repeatly, until the third tap to stop.

In other words, the mechanism is similar to tap the tempo on synthesizer.
Record the tempo, playback, then stop.

## How to record
- 1st Press (Start recording `tap-hold` duration)
- 1st Release (Rec `tap-hold` duration and start recording `delay` duration))
- 2nd Press (Rec `delay` duration and start playback `tap-hold` and `delay` combo)
- 3rd Press (Stop loop playback)

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

Now, update your `shield.keymap` adding the behaviors. You can check the `usage-pages` on [Key Repeat](https://zmk.dev/docs/behaviors/key-repeat) doc page.

```keymap
#include <behaviors/tempo.dtsi>
/ {
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
