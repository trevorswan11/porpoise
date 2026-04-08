R"(ExpressionStatement
└─ Expr: ArrayExpression
    ├─ Size: (inferred)
    ├─ Type: ExplicitType (modifier: PTR)
    │   └─ IdentifierExpression: N
    └─ Items:
        ├─ IdentifierExpression: a
        ├─ IdentifierExpression: b
        ├─ IdentifierExpression: c
        ├─ IdentifierExpression: d
        ├─ IdentifierExpression: e
        ├─ I32Expression: 3
        └─ StringExpression: 54
ExpressionStatement
└─ Expr: BinaryExpression (BOOLEAN_AND)
    ├─ LHS: BinaryExpression (BOOLEAN_OR)
    │   ├─ LHS: BinaryExpression (LT_EQ)
    │   │   ├─ LHS: IdentifierExpression: a
    │   │   └─ RHS: IdentifierExpression: b
    │   └─ RHS: BinaryExpression (EQ)
    │       ├─ LHS: IdentifierExpression: c
    │       └─ RHS: IdentifierExpression: d
    └─ RHS: IdentifierExpression: e
ExpressionStatement
└─ Expr: BinaryExpression (BOOLEAN_OR)
    ├─ LHS: IdentifierExpression: a
    └─ RHS: BinaryExpression (EQ)
        ├─ LHS: IndexExpression
        │   ├─ Object: IdentifierExpression: b
        │   └─ Index: USizeExpression: 3
        └─ RHS: UnaryExpression (BANG)
            └─ Operand: IdentifierExpression: c
JumpStatement (CONTINUE)
JumpStatement (RETURN)
└─ Value: EnumExpression
    └─ Enumerations:
        └─ Name: IdentifierExpression: RED
ImportStatement
└─ Library: IdentifierExpression: std
ImportStatement
├─ File: StringExpression: ast/node.p
└─ Alias: IdentifierExpression: node
DiscardStatement
└─ Discarded: EnumExpression
    └─ Enumerations:
        └─ Name: IdentifierExpression: RED
DeclStatement (SIZE)
├─ Modifiers: CONSTEXPR
├─ Type: (inferred)
└─ Value: USizeExpression: 2
BlockStatement
├─ ExpressionStatement
│   └─ Expr: IdentifierExpression: a
├─ ExpressionStatement
│   └─ Expr: IdentifierExpression: b
├─ ExpressionStatement
│   └─ Expr: I32Expression: 2
└─ ExpressionStatement
    └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: WhileLoopExpression
    ├─ Condition: BoolExpression: true
    ├─ Continuation: AssignmentExpression (PLUS_ASSIGN)
    │   ├─ Assignee: IdentifierExpression: i
    │   └─ Value: I32Expression: 1
    ├─ Body: BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: a
    └─ Non-Break Clause: JumpStatement (RETURN)
        └─ Value: IdentifierExpression: b
DeclStatement (f_ptr)
├─ Modifiers: VARIABLE
└─ Type: ExplicitType (modifier: PTR)
    └─ FunctionExpression
        ├─ Parameters:
        │   ├─ Param:
        │   │   └─ Type: ExplicitType (modifier: REF)
        │   │       └─ IdentifierExpression: a
        │   └─ Param:
        │       └─ Type: ExplicitType (modifier: MUT_PTR)
        │           └─ IdentifierExpression: B
        ├─ Variadic: true
        └─ Returns: ExplicitType (modifier: REF)
            └─ ArrayType
                ├─ Dimensions: USizeExpression: 2
                ├─ Null terminated: false
                └─ ExplicitType (modifier: BASE)
                    └─ ArrayType
                        ├─ Dimensions: IdentifierExpression: N
                        ├─ Null terminated: false
                        └─ ExplicitType (modifier: PTR)
                            └─ IdentifierExpression: E
ExpressionStatement
└─ Expr: ScopeResolutionExpression
    ├─ Outer: ScopeResolutionExpression
    │   ├─ Outer: IdentifierExpression: A
    │   └─ Inner: IdentifierExpression: B
    └─ Inner: IdentifierExpression: C
ExpressionStatement
└─ Expr: StructExpression (packed)
    ├─ DeclStatement (a)
    │   ├─ Modifiers: VARIABLE
    │   ├─ Type: ExplicitType (modifier: BASE)
    │   │   └─ IdentifierExpression: Foo
    │   └─ Value: IdentifierExpression: bar
    └─ DeclStatement (b)
        ├─ Modifiers: CONSTANT
        ├─ Type: (inferred)
        └─ Value: FunctionExpression
            ├─ Self: this (modifier: MUT_PTR)
            ├─ Parameters:
            │   ├─ Param:
            │   │   ├─ Name: IdentifierExpression: a
            │   │   └─ Type: ExplicitType (modifier: BASE)
            │   │       └─ IdentifierExpression: A
            │   └─ Param:
            │       ├─ Name: IdentifierExpression: b
            │       └─ Type: ExplicitType (modifier: PTR)
            │           └─ IdentifierExpression: B
            ├─ Variadic: false
            ├─ Returns: ExplicitType (modifier: BASE)
            │   └─ IdentifierExpression: C
            └─ Body: BlockStatement
                └─ ExpressionStatement
                    └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: ReferenceExpression (BW_AND)
    └─ Operand: IdentifierExpression: a
ExpressionStatement
└─ Expr: ReferenceExpression (AND_MUT)
    └─ Operand: IdentifierExpression: b
ExpressionStatement
└─ Expr: DereferenceExpression (STAR)
    └─ Operand: IdentifierExpression: a
ExpressionStatement
└─ Expr: MatchExpression
    ├─ Matcher: IdentifierExpression: a
    ├─ Arms:
    │   ├─ Arm:
    │   │   ├─ Pattern: IdentifierExpression: b
    │   │   ├─ Capture: IdentifierExpression: c
    │   │   └─ Dispatch: ExpressionStatement
    │   │       └─ Expr: IdentifierExpression: d
    │   ├─ Arm:
    │   │   ├─ Pattern: IdentifierExpression: e
    │   │   ├─ Capture: <discarded>
    │   │   └─ Dispatch: ExpressionStatement
    │   │       └─ Expr: IdentifierExpression: f
    │   └─ Arm:
    │       ├─ Pattern: IdentifierExpression: g
    │       └─ Dispatch: ExpressionStatement
    │           └─ Expr: IdentifierExpression: h
    └─ Catch All: ExpressionStatement
        └─ Expr: IdentifierExpression: d
ExpressionStatement
└─ Expr: InfiniteLoopExpression
    └─ ExpressionStatement
        └─ Expr: IdentifierExpression: a
ExpressionStatement
└─ Expr: AssignmentExpression (ASSIGN)
    ├─ Assignee: DereferenceExpression (STAR)
    │   └─ Operand: IndexExpression
    │       ├─ Object: IndexExpression
    │       │   ├─ Object: IdentifierExpression: arr
    │       │   └─ Index: IdentifierExpression: i
    │       └─ Index: IdentifierExpression: j
    └─ Value: I32Expression: 2
ExpressionStatement
└─ Expr: IfExpression
    ├─ Constexpr: false
    ├─ Condition: IdentifierExpression: a
    ├─ Consequence: BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: b
    └─ Alternate: BlockStatement
        └─ ExpressionStatement
            └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: IfExpression
    ├─ Constexpr: true
    ├─ Condition: IdentifierExpression: a
    └─ Consequence: BlockStatement
        └─ ExpressionStatement
            └─ Expr: IdentifierExpression: b
ExpressionStatement
└─ Expr: FunctionExpression
    ├─ Self: this (modifier: MUT_PTR)
    ├─ Parameters:
    │   ├─ Param:
    │   │   ├─ Name: IdentifierExpression: a
    │   │   └─ Type: ExplicitType (modifier: BASE)
    │   │       └─ IdentifierExpression: A
    │   └─ Param:
    │       ├─ Name: IdentifierExpression: b
    │       └─ Type: ExplicitType (modifier: PTR)
    │           └─ IdentifierExpression: B
    ├─ Variadic: false
    ├─ Returns: ExplicitType (modifier: BASE)
    │   └─ IdentifierExpression: i32 (primitive)
    └─ Body: BlockStatement
        └─ ExpressionStatement
            └─ Expr: IdentifierExpression: c
ExpressionStatement
└─ Expr: ForLoopExpression
    ├─ Iterables:
    │   ├─ IdentifierExpression: arr
    │   ├─ IdentifierExpression: l
    │   └─ IdentifierExpression: p
    ├─ Captures:
    │   ├─ i (modifier: BASE)
    │   ├─ j (modifier: MUT_REF)
    │   └─ <discarded>
    ├─ Body: BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: a
    └─ Non-Break: JumpStatement (RETURN)
        └─ Value: IdentifierExpression: b
ExpressionStatement
└─ Expr: EnumExpression
    ├─ Underlying: IdentifierExpression: u64 (primitive)
    └─ Enumerations:
        ├─ Name: IdentifierExpression: A
        │   └─ Default: U64Expression: 1
        ├─ Name: IdentifierExpression: B
        │   └─ Default: IdentifierExpression: T
        └─ Name: IdentifierExpression: C
ExpressionStatement
└─ Expr: CallExpression
    ├─ Callee: IdentifierExpression: @ptrAdd (builtin)
    └─ Arguments:
        ├─ IdentifierExpression: a
        └─ USizeExpression: 4
UsingStatement
├─ Alias: IdentifierExpression: T
└─ Type: ExplicitType (modifier: BASE)
    └─ IdentifierExpression: i32 (primitive)
ExpressionStatement
└─ Expr: CallExpression
    ├─ Callee: IdentifierExpression: a
    └─ Arguments:
        ├─ ReferenceExpression (AND_MUT)
        │   └─ Operand: IdentifierExpression: r
        ├─ IdentifierExpression: t
        └─ ExplicitType (modifier: PTR)
            └─ ArrayType
                ├─ Dimensions: IdentifierExpression: N
                ├─ Null terminated: true
                └─ ExplicitType (modifier: BASE)
                    └─ IdentifierExpression: u8 (primitive)
ExpressionStatement
└─ Expr: ImplicitAccessExpression (DOT)
    └─ Operand: IdentifierExpression: a
ExpressionStatement
└─ Expr: UnionExpression
    ├─ Field:
    │   ├─ Tag: IdentifierExpression: a
    │   └─ Type: ExplicitType (modifier: BASE)
    │       └─ IdentifierExpression: i32 (primitive)
    └─ Field:
        ├─ Tag: IdentifierExpression: b
        └─ Type: ExplicitType (modifier: MUT_REF)
            └─ IdentifierExpression: T
ExpressionStatement
└─ Expr: DoWhileLoopExpression
    ├─ Body: BlockStatement
    │   └─ ExpressionStatement
    │       └─ Expr: IdentifierExpression: a
    └─ Condition: BoolExpression: true
ExpressionStatement
└─ Expr: I64Expression: 1
ExpressionStatement
└─ Expr: ISizeExpression: 2
ExpressionStatement
└─ Expr: U32Expression: 3
ExpressionStatement
└─ Expr: U8Expression: a
ExpressionStatement
└─ Expr: F32Expression: 2.3
ExpressionStatement
└─ Expr: F64Expression: 2.3
ExpressionStatement
└─ Expr: ArrayExpression
    ├─ Size: USizeExpression: 1
    ├─ Type: ExplicitType (modifier: BASE)
    │   └─ IdentifierExpression: A
    └─ Items:
        └─ IdentifierExpression: a
ExpressionStatement
└─ Expr: DotExpression (DOT)
    ├─ Object: IdentifierExpression: a
    └─ Member: IdentifierExpression: b
ExpressionStatement
└─ Expr: RangeExpression (DOT_DOT)
    ├─ Lower: IdentifierExpression: a
    └─ Upper: IdentifierExpression: b
ExpressionStatement
└─ Expr: RangeExpression (DOT_DOT_EQ)
    ├─ Lower: IdentifierExpression: a
    └─ Upper: IdentifierExpression: b
DeclStatement (a)
├─ Modifiers: VARIABLE
└─ Type: ExplicitType (modifier: BASE)
    └─ ArrayType
        ├─ Dimensions: (slice)
        ├─ Null terminated: false
        └─ ExplicitType (modifier: BASE)
            └─ IdentifierExpression: i32 (primitive)
BlockStatement
└─ <empty>
DeferStatement
└─ Deferred: ExpressionStatement
    └─ Expr: I32Expression: 3
ExpressionStatement
└─ Expr: Initializer Expression:
    ├─ Object Type: <inferred>
    └─ Initializers:
        └─ Initializer:
            ├─ Member: ImplicitAccessExpression (DOT)
            │   └─ Operand: IdentifierExpression: a
            └─ Value: I32Expression: 3
ExpressionStatement
└─ Expr: Initializer Expression:
    ├─ Object Type: IdentifierExpression: TT
    └─ Initializers:
        └─ Initializer:
            ├─ Member: ImplicitAccessExpression (DOT)
            │   └─ Operand: IdentifierExpression: adfasf
            └─ Value: IdentifierExpression: a
)"
