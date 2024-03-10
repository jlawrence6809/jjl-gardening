For parsing the automation instructions I played around with Antlr (HomeAutomation.g4) and you can generate the javascript files with the following command:

```bash
antlr -Dlanguage=JavaScript HomeAutomation.g4
```

But... the file were too big and I didn't want to load them onto the esp32. So I decided to write my own parser. It's not as powerful as Antlr, but it's good enough for my use case.
