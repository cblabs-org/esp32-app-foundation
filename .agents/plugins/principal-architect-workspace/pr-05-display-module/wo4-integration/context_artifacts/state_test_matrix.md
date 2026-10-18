# State: WO4 Test Matrix — 87 Tests

## Test Files

### test_boards.cpp (45 tests — from WO2/WO3, unchanged)

| Fixture | Test | Assertion |
|---|---|---|
| BootSplashBoardTest | GetBoardName_ReturnsCorrectName | STREQ "SYS: Boot Splash" |
| BootSplashBoardTest | IsExpired_BeforeActivate_ReturnsFalse | FALSE |
| BootSplashBoardTest | IsExpired_AfterActivate_ReturnsFalse | FALSE |
| BootSplashBoardTest | Activate_NullVersion_DoesNotCrash | NO_FATAL_FAILURE |
| BootSplashBoardTest | Activate_EmptyVersion_DoesNotCrash | NO_FATAL_FAILURE |
| LoadingBoardTest | GetBoardName_ReturnsCorrectName | STREQ "SYS: Loading" |
| LoadingBoardTest | IsExpired_Initially_ReturnsFalse | FALSE |
| LoadingBoardTest | SetComplete_CausesExpiry | TRUE |
| LoadingBoardTest | SetStatus_ValidInput_DoesNotCrash | NO_FATAL_FAILURE |
| LoadingBoardTest | SetStatus_NullMessage_DoesNotCrash | NO_FATAL_FAILURE |
| LoadingBoardTest | SetStatus_ProgressClamping | NO_FATAL_FAILURE |
| LoadingBoardTest | SetComplete_ThenSetStatus_RemainsExpired | TRUE |
| WiFiWizardBoardTest | GetBoardName_ReturnsCorrectName | STREQ "SYS: WiFi Wizard" |
| WiFiWizardBoardTest | IsExpired_Initially_ReturnsFalse | FALSE |
| WiFiWizardBoardTest | Dismiss_CausesExpiry | TRUE |
| WiFiWizardBoardTest | Configure_ValidInputs_DoesNotCrash | NO_FATAL_FAILURE |
| WiFiWizardBoardTest | Configure_NullSSID_DoesNotCrash | NO_FATAL_FAILURE |
| WiFiWizardBoardTest | Configure_DefaultIP_DoesNotCrash | NO_FATAL_FAILURE |
| WiFiWizardBoardTest | Configure_LongSSID_Truncates | NO_FATAL_FAILURE |
| OtaProgressBoardTest | GetBoardName_ReturnsCorrectName | STREQ "SYS: OTA Progress" |
| OtaProgressBoardTest | IsExpired_Initially_ReturnsFalse | FALSE |
| OtaProgressBoardTest | SetProgress_ValidInput_DoesNotCrash | NO_FATAL_FAILURE |
| OtaProgressBoardTest | SetProgress_NullStatus_DoesNotCrash | NO_FATAL_FAILURE |
| OtaProgressBoardTest | SetComplete_Success_CausesExpiry | TRUE |
| OtaProgressBoardTest | SetComplete_Failure_CausesExpiry | TRUE |
| OtaProgressBoardTest | SetProgress_ClampsToRange | NO_FATAL_FAILURE |
| OtaProgressBoardTest | SetProgress_AfterComplete_DoesNotCrash | NO_FATAL_FAILURE |
| ErrorBoardTest | GetBoardName_ReturnsCorrectName | STREQ "SYS: Error" |
| ErrorBoardTest | IsExpired_Initially_ReturnsTrue | TRUE |
| ErrorBoardTest | ShowError_NonFatal_DoesNotExpire | FALSE |
| ErrorBoardTest | ShowError_Fatal_DoesNotExpire | FALSE |
| ErrorBoardTest | Dismiss_AfterShow_CausesExpiry | TRUE |
| ErrorBoardTest | ShowError_NullMessage_DoesNotCrash | NO_FATAL_FAILURE |
| ErrorBoardTest | ShowError_EmptyMessage_DoesNotCrash | NO_FATAL_FAILURE |
| ErrorBoardTest | ShowError_LongMessage_Truncates | NO_FATAL_FAILURE |
| ErrorBoardTest | Dismiss_WithoutShow_DoesNotCrash | NO_FATAL_FAILURE |
| ScreensaverBoardTest | GetBoardName_ReturnsCorrectName | STREQ "SYS: Screensaver" |
| ScreensaverBoardTest | IsExpired_Initially_ReturnsTrue | TRUE |
| ScreensaverBoardTest | Activate_ClearsExpiry | FALSE |
| ScreensaverBoardTest | Deactivate_CausesExpiry | TRUE |
| ScreensaverBoardTest | Activate_ThenDeactivate_Expires | TRUE |
| ScreensaverBoardTest | MultipleActivations_DoNotCrash | NO_FATAL_FAILURE |
| ScreensaverBoardTest | Deactivate_WithoutActivate_DoesNotCrash | NO_FATAL_FAILURE |
| ScreensaverBoardTest | Activate_AfterDeactivate_ClearsExpiry | FALSE |
| ScreensaverBoardTest | MultipleDeactivations_DoNotCrash | NO_FATAL_FAILURE |

### test_carousel.cpp (11 tests — WO4)

| Fixture | Test | Assertion |
|---|---|---|
| CarouselSlotPoolTest | InitialBoardCount_IsZero | EQ 0 |
| CarouselSlotPoolTest | AddBoard_ValidPointer_ReturnsTrue | TRUE, EQ 1 |
| CarouselSlotPoolTest | AddBoard_Nullptr_ReturnsFalse | FALSE, EQ 0 |
| CarouselSlotPoolTest | AddBoard_Multiple_IncrementCount | TRUE ×3, EQ 3 |
| CarouselSlotPoolTest | GetActiveBoard_EmptyPool_ReturnsNullptr | EQ nullptr |
| CarouselSlotPoolTest | GetActiveBoard_OneBoard_ReturnsThatBoard | EQ &board |
| CarouselShowBoardTest | ShowBoard_PoolBoard_UpdatesActiveBoard | EQ &boardB |
| CarouselShowBoardTest | ShowBoard_NonPoolBoard_SetsOverride | EQ &systemBoard |
| CarouselShowBoardTest | ShowBoard_BackToPoolBoard_ClearsOverride | EQ &poolBoard |
| CarouselEvictionTest | Tick_EmptyPool_DoesNotCrash | NO_FATAL_FAILURE |
| CarouselEvictionTest | Tick_WithBoards_DoesNotCrash | NO_FATAL_FAILURE |
| CarouselEvictionTest | GetBoardCount_TracksCorrectly | EQ 0→1→2 |

### test_status_reporter.cpp (11 tests — WO4)

| Fixture | Test | Assertion |
|---|---|---|
| StatusReporterTest | ReportStatus_DoesNotCrash | NO_FATAL_FAILURE |
| StatusReporterTest | ReportStatus_ShowsLoadingBoard | EQ &getLoading() |
| StatusReporterTest | ReportStatus_IndeterminateProgress | NO_FATAL_FAILURE, EQ |
| StatusReporterTest | ReportStatus_NullMessage_DoesNotCrash | NO_FATAL_FAILURE |
| StatusReporterTest | ReportStatus_MultipleCalls_KeepsLoadingActive | EQ &getLoading() |
| StatusReporterTest | ReportError_DoesNotCrash | NO_FATAL_FAILURE |
| StatusReporterTest | ReportError_ShowsErrorBoard | EQ &getError() |
| StatusReporterTest | ReportError_NullMessage_DoesNotCrash | NO_FATAL_FAILURE |
| StatusReporterTest | ClearStatus_DoesNotCrash | NO_FATAL_FAILURE |
| StatusReporterTest | ClearStatus_MarksLoadingComplete | TRUE isExpired() |
| StatusReporterTest | ReportError_OverridesLoadingBoard | EQ &getError() |

### test_config.cpp (10 tests — WO4)

| Fixture | Test | Assertion |
|---|---|---|
| ConfigTest | LoadConfig_NativeBuild_ReturnsTrue | TRUE |
| ConfigTest | SaveConfig_NativeBuild_ReturnsTrue | TRUE |
| ConfigTest | ResetConfig_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigTest | ResetConfig_ThenLoadConfig_ReturnsTrue | TRUE |
| ConfigTest | ResetConfig_ThenSaveConfig_ReturnsTrue | TRUE |
| ConfigSettersTest | SetBrightness_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetBrightness_Zero_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetBrightness_Max_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetFlipScreen_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetSleepTimeout_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetSleepTimeout_Zero_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetRotationInterval_DoesNotCrash | NO_FATAL_FAILURE |
| ConfigSettersTest | SetRotationInterval_SmallValue_DoesNotCrash | NO_FATAL_FAILURE |

### test_eventbus_integration.cpp (10 tests — WO4)

| Fixture | Test | Assertion |
|---|---|---|
| PayloadStructTest | OtaProgressPayload_HasExpectedFields | FLOAT_EQ, STREQ, FALSE |
| PayloadStructTest | OtaProgressPayload_CompleteSuccess | FLOAT_EQ, TRUE ×2 |
| PayloadStructTest | OtaProgressPayload_CompleteFailure | FLOAT_EQ, TRUE, FALSE |
| PayloadStructTest | DisplayMessagePayload_HasExpectedFields | STREQ, EQ 5000 |
| PayloadStructTest | DisplayMessagePayload_ZeroDuration | STREQ, EQ 0 |
| EventBusIntegrationTest | OtaProgress_Publish_DelegatesToBoard | EQ 1 delivered, EQ &otaProgress |
| EventBusIntegrationTest | OtaProgress_Complete_Success_BoardExpires | TRUE isExpired() |
| EventBusIntegrationTest | DisplayMessage_Publish_ShowsMessage | EQ 1 delivered, EQ &loading |
| EventBusIntegrationTest | Publish_NoSubscribers_ReturnsZero | EQ 0 |
