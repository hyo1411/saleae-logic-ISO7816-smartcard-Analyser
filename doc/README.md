[installation]: img/installation.png "Installation"
[configuration]: img/configuration.png "Signal lines configuration"
[t1-sblock]: img/t1-s_r-blocks.png "T1 S/R blocks"
[t1-lrc]: img/t1-lrc-validation.png "T1 LRC validation"
[t1-support]: img/t1-support.png "T1 frames decoding"
[pps]: img/pps-decoding.png "PPS decoding"
[bits-decoration]: img/bits-decoration.png "Bits decoration"
[atr-decoding]: img/atr-byte-decoration.png "ATR bytes decoration"
[atr-pps]: img/atr-pps.png "ATR / PPS decoding"
[reset-detection]: img/reset-detection.png "Reset detection"


Before the first use the plugin has to be copied to Saleae analyzers folder and configured.
You can download a compiled version directly from GitHub: [bin folder](../bin/)

and upload it to the right folder as shown below:
![analyzer's folder][installation]

After that signal lines have to be selected as shown on the following screenshot:
![Signal lines configuration][configuration]
This configuration can be changed at any time by using configuration icon - in the Analyzers section.

Then the analysis can be started.
The first step to start analysis is to detect RESET signal. The plugin supports not only cold but also warm reset:
![Reset detection][reset-detection]

After that an ATR and optional PPS exchange is decoded:
![ATR / PPS decoding][atr-pps]

Zooming in the view more details can be seen:
![Decoration of ATR's bytes][atr-decoding]

Based on ATR, plugin is able to determine card mode - negotiable or specific. If negotiable mode is detected,
the plugin waits for an optional PPS exchange:
![PPS exchange][pps]

Based on ATR/PPS plugin determines further communication parameters such as Fi/Di and protocol:

If T1 protocol is selected then suitable frames will be decoded and shown:
![T1 I-block frame][t1-support]

For each T1 frame LRC checksum is verified and the result presented as '-OK' or '-ERR':
![LRC verification][t1-lrc]
**For the time being CRC checksum is not supported yet!**

Also S/R frames are supported, but no details about their content is presented:
![T1 sample S-block][t1-sblock]

