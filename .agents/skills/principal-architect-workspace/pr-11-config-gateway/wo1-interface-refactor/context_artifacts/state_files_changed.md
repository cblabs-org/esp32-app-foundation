# State: Files Changed in WO1

## Interfaces
| File | Change |
|---|---|
| `include/interfaces/iConfigurable.hpp` | Redesigned: 6-method contract (getConfigNamespace, getSchemaVersion, applyConfig, serializeConfig, resetConfig, migrateConfig) |
| `include/interfaces/IWebContributor.hpp` | Added: `getParentTab()` and `getTabOrder()` virtual methods with defaults |

## ConfigManager
| File | Change |
|---|---|
| `modules/configManager/configManager.hpp` | Complete rewrite: fixed-size registry, gateway API, IWebContributor impl |
| `modules/configManager/configManager.cpp` | Complete rewrite: beginAll, saveModuleConfig, exportAllConfig, importAllConfig, registerRoutes |

## Module Migrations
| File | Change |
|---|---|
| `modules/wifiManager/wifiManager.hpp` | Implements iConfigurable (ns: "wifi", v1), getTabOrder(10) |
| `modules/wifiManager/wifiManager.cpp` | applyConfig/serializeConfig/resetConfig; removed direct LittleFS |
| `modules/u8g2Display/u8g2DisplayModule.hpp` | Implements iConfigurable (ns: "display", v1), getTabOrder(20) |
| `modules/u8g2Display/u8g2DisplayModule.cpp` | applyConfig/serializeConfig/resetConfig; removed direct LittleFS |

## Application
| File | Change |
|---|---|
| `modules/application/Application.hpp` | Removed _configurables vector, added ConfigManager member, getConfigManager() |
| `modules/application/Application.cpp` | Replaced registerConfigurable/loadConfig loop with _configManager.beginAll() |

## Supporting Modules
| File | Change |
|---|---|
| `modules/webServer/webServer.cpp` | Tabs endpoint includes parent/order fields |
| `modules/mcpServer/mcpBuiltinTools.cpp` | get_config/set_config use gateway export/import instead of SystemConfig |

## Tests
| File | Change |
|---|---|
| `test/test_unit/test_application/test_application.cpp` | Removed registerConfigurable tests, added ConfigManagerGatewayTest group |
| `test/test_unit/test_configManager/test_configManager.cpp` | Complete rewrite for gateway pattern |
| `test/test_unit/test_u8g2Display/test_config.cpp` | Updated to 6-method iConfigurable interface |
| `test/test_unit/test_wifiManager/test_wifiManager.cpp` | Added iConfigurable + tabOrder tests |
