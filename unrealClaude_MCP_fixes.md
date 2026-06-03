# unrealClaude MCP — Known Errors & Fixes

## enhanced_input domain

### Operations
| Wrong | Correct |
|-------|---------|
| `create_action` | `create_input_action` |

### Value Types (create_input_action)
Valid: `Digital`, `Boolean`, `Bool`, `Axis1D`, `Float`, `Axis2D`, `Vector2D`, `Axis3D`, `Vector`
- ❌ `"Axis2D (Vector2D)"` — parenthetical form INVALID
- ✅ `"Axis2D"` or `"Vector2D"`

### remove_mapping
- ❌ param `index` → NOT valid
- ❌ params `action_path` + `key` → NOT valid
- ✅ param MUST be `mapping_index` (integer)

---

## blueprint domain

### Node Types (add_node / add_nodes)
Supported: `CallFunction`, `Branch`, `Event`, `VariableGet`, `VariableSet`, `Sequence`, `Add`, `Subtract`, `Multiply`, `Divide`, `PrintString`, `EnhancedInputAction`
- ❌ `K2Node_IfThenElse` → use `Branch`
- ❌ `K2Node_*` prefix never works — use short names only

### VariableGet / VariableSet node param key
- ❌ `node_params: { variable_name: "x" }` → FAILS "Variable name is required"
- ❌ `params: { variable_name: "x" }` → FAILS same
- ✅ Try top-level: `{ "type": "VariableGet", "variable_name": "bVideoModeActive", ... }`
  (use `add_node` singular, not `add_nodes`)

### Math node types
- ❌ `node_type: "Multiply"` with `node_params: { type: "float" }` → internally looks for `Multiply_FloatFloat`, not found
- ✅ Use `CallFunction` + `"function": "Multiply_FloatFloat"` + `"class": "KismetMathLibrary"`

### connect_pins — compile error "Replace existing output connections"
- Cause: target execution pin already connected to another node
- Fix: disconnect existing connection first before wiring new one

---

## Functions NOT found (CallFunction) — Workarounds

| Function Tried | All Classes Tried | Status | Workaround |
|---|---|---|---|
| `GetWorldDeltaSeconds` | KismetSystemLibrary, GameplayStatics | ❌ Not found | Use **Event Tick's DeltaSeconds pin directly** — no function call needed |
| `GetPlayerController` | GameplayStatics, KismetGameplayStatics, Actor | ❌ Not found | In PlayerController subclass BP: use **Self** reference directly |
| `SetShowMouseCursor` | PlayerController | ❌ Not a function | Bool UPROPERTY — need property setter approach; manually wire in editor |
| `SetInputMode_GameOnly` | KismetInputLibrary, WidgetBlueprintLibrary, PlayerController | ❌ Not found | Manually wire in editor |
| `SetInputMode_GameAndUIOnly` | WidgetBlueprintLibrary | ❌ Not found | Manually wire in editor |
| `AddMappingContext` | EnhancedInputLocalPlayerSubsystem, EnhancedInputSubsystemInterface, K2 variants | ❌ Not found | **Use enhanced_input domain** `add_mapping` operation instead; or wire manually |
| `RemoveMappingContext` | EnhancedInputSubsystemInterface | ❌ Not found | Use enhanced_input domain `remove_mapping` with `mapping_index` |

---

## EnhancedInputAction node

### Setting the Input Action asset on node
- ❌ `set_pin_value` with `pin_name: "InputAction"` → pin not found
- ✅ Set action during node creation via `node_params` or `params` at creation time
- ✅ Or use `"node_type": "EnhancedInputAction"` with `"action_path": "/Game/Controls/IA_Move"`

---

## General Rules Learned

1. **Query before creating** — use `query_context`, `query_action`, `list_actions` to verify asset paths exist
2. **Math functions**: use `CallFunction` + `KismetMathLibrary` class, NOT shorthand node types
3. **Subsystem functions** (AddMappingContext etc): MCP CallFunction cannot reach subsystem instance methods — use enhanced_input domain operations instead
4. **PlayerController properties** (ShowMouseCursor, InputMode): cannot be set via CallFunction — wire manually in editor
5. **DeltaSeconds**: always pull from Event Tick pin, never call a function
6. **add_nodes (batch) vs add_node (singular)**: param key schema differs — prefer `add_node` singular when hitting variable name issues
