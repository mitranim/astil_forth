```forth

1 0 extern: puts

: main " hello" puts ;

```


```
Compiled code
────────────────────

  call through GOT slot
          │
          ▼
  ┌────────────────────┐
  │ __DATA_CONST __got │
  │                    │
  │ slot for _puts     │
  └─────────┬──────────┘
            │
            │ described by
            ▼
  ┌────────────────────────────┐
  │ __LINKEDIT                 │
  │ chained fixup metadata     │
  │ symbol name: _puts         │
  │ library: libSystem         │
  └─────────┬──────────────────┘
            │
            │ patched by macOS dyld
            ▼
  ┌───────────────────────┐
  │ actual addr of `puts` │
  └───────────────────────┘
```
