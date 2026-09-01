import re
with open("modules/Application/Application.cpp", "r") as f:
    content = f.read()

# Add headers if not present
if "CompositeSanitizer.h" not in content:
    content = content.replace('#include "LogPipeline.h"', '#include "LogPipeline.h"\n#include "CompositeSanitizer.h"\n#include "StructuralScrubber.h"')

# Replace LogPipeline::begin
target = 'LogPipeline::begin(&Redactor::instance(), [](const char* str) -> int { return fputs(str, stdout); });'
replacement = '''static CompositeSanitizer s_compositeSanitizer;
    s_compositeSanitizer.add(&Redactor::instance());
#if defined(CONFIG_LOG_MAXIMUM_LEVEL) && CONFIG_LOG_MAXIMUM_LEVEL >= 4
    s_compositeSanitizer.add(&StructuralScrubber::instance());
#endif
    LogPipeline::begin(&s_compositeSanitizer, [](const char* str) -> int { return fputs(str, stdout); });'''

if target in content:
    content = content.replace(target, replacement)
    with open("modules/Application/Application.cpp", "w") as f:
        f.write(content)
    print("Patched successfully")
else:
    print("Target not found")
