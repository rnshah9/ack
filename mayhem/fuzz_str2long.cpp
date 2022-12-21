#include <stdint.h>
#include <stdio.h>
#include <climits>

#include <fuzzer/FuzzedDataProvider.h>

extern "C" long str2long(char *s, int b);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    char* s = strdup(provider.ConsumeRandomLengthString().c_str());
    int b = provider.ConsumeIntegral<int>();

    str2long(s, b);

    free(s);
    return 0;
}
