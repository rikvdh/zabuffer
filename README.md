# zabuffer
Zero-Allocation buffer handling in C

## Installation


With [clib](https://github.com/clibs/clib):

```sh
clib install rikvdh/zabuffer
```

## Example

```c
#include "zabuffer.h"
#include <assert.h>

// Create buffer called 'buf' with size of 100 bytes
ZA_BUFFER_DECL(buf, 100)

int main(int argc, char **argv)
{
    za_buffer_write_data(buf, "hello", 5);

    assert(5 == za_buffer_size_inuse(buf));

    za_buffer_flush(&buf);

    return 0;
}
```