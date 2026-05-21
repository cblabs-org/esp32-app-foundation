# Graveyard: Entry Point & Façade

## CONFIG_AUTOSTART_ARDUINO Missing
During hardware build verification of the entry point, the build initially failed because `CONFIG_AUTOSTART_ARDUINO=y` was missing from `sdkconfig.defaults`. In dual-framework projects, this parameter is required to allow PlatformIO to link the Arduino initialization code with ESP-IDF's bootstrap wrapper.
- **Lesson Learned**: Ensure `CONFIG_AUTOSTART_ARDUINO=y` is explicitly added to sdkconfig templates for all dual-framework projects.
