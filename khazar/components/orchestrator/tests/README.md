# tests — Unit and Integration Tests

## Running
```bash
make test
```

## Test Files
- `test_logger.c` — Logger module (levels, formatting)
- `test_protocol.c` — Protocol module (parse, build)
- `test_registry.c` — Registry module (register, lookup, unregister)
- `test_policy.c` — Policy engine (allow/deny rules)
- Integration tests via `test_integration.sh`
