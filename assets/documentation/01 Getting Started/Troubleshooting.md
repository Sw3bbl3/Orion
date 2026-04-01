# Troubleshooting & FAQ

If you encounter issues while using Orion, check this guide for quick solutions to the most common problems.

## Audio Issues

### I hear pops and clicks (Buffer Underruns)
- **Cause**: CPU is working too hard to process audio in real-time.
- **Solution**: Increase your **Buffer Size** in **Settings > Audio**. If it's at 128, try 256 or 512.

### There's a delay when I play (Latency)
- **Cause**: Buffer size is too large or a high-latency plugin is on the signal path.
- **Solution**: Decrease **Buffer Size**. Also, check for "Mastering" plugins on the master bus, as these often introduce latency.

---

## MIDI Issues

### My keyboard isn't making sound
- **Connection**: Ensure the USB/MIDI cable is secure.
- **Settings**: Check **Settings > MIDI** and ensure your device is checked.
- **Track Arm**: Ensure the track you want to play is **Armed (R)**.
- **Monitor**: Ensure **Input Monitor** is on.

---

## Application Issues

### Orion is sluggish or freezing
- **Extensions**: A heavy or poorly-written Lua extension might be blocking the UI thread.
- **Plugin Management**: Try disabling third-party plugins one by one to identify the culprit.
- **Logs**: Check the Orion Log file (available via `Cmd/Ctrl + L`).

---

## Frequently Asked Questions

**Q: Where are my projects saved?**
A: By default, they are in `Documents/Orion/Projects`, but you can change this in **Settings > Folders**.

**Q: Can I use VST3 plugins?**
A: Yes! Orion fully supports VST3 and AU plugins. Ensure your scan paths are set in **Settings > Plugins**.

**Q: How do I export for SoundCloud/Spotify?**
A: Use the **Export Render** dialog (`Cmd/Ctrl + E`) and select the "WAV (24-bit PCM)" format with a 44.1kHz sample rate.

> [!TIP]
> If you're still stuck, join our [Discord Community](https://discord.gg/orion-daw) for real-time support from other users and the dev team.
