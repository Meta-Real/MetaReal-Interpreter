/*/
 * MetaReal version 1.0.0
/*/

#include <parser/parser.h>
#include <memory.h>
#include <stdlib.h>
#include <def.h>

#define advance_newline(t) if (t->type == NEWLINE_T) t++

void pres_fail(pres_p pres, invalid_syntax_p error);

token_p dollar_func(pres_p pres, token_p tokens); // dollar-function
token_p statement(pres_p res, token_p tokens);    // return continue break
token_p tuple(pres_p res, token_p tokens);        // tuple
token_p assign(pres_p res, token_p tokens);       // = += -= *= /= %= //= **= &= |= ^= <<= >>=
token_p ternary(pres_p res, token_p tokens);      // ?:
token_p typeof(pres_p pres, token_p tokens);      // is are
token_p contain(pres_p pres, token_p tokens);     // in
token_p or(pres_p pres, token_p tokens);          // || or
token_p xor(pres_p pres, token_p tokens);         // ^^ xor
token_p and(pres_p pres, token_p tokens);         // && and
token_p compare1(pres_p pres, token_p tokens);    // == !=
token_p compare2(pres_p pres, token_p tokens);    // < > <= >=
token_p b_or(pres_p pres, token_p tokens);        // |
token_p b_xor(pres_p pres, token_p tokens);       // ^
token_p b_and(pres_p pres, token_p tokens);       // &
token_p shift(pres_p pres, token_p tokens);       // << >>
token_p expression(pres_p pres, token_p tokens);  // + -
token_p term(pres_p pres, token_p tokens);        // * / % //
token_p factor(pres_p pres, token_p tokens);      // +(unary) -(unary) ~ ! not ++ --
token_p exponent(pres_p pres, token_p tokens);    // **
token_p post(pres_p pres, token_p tokens);        // array-subscripting access ++(post) --(post) function-call
token_p core(pres_p pres, token_p tokens);        // types statements

token_p list_parse(pres_p pres, token_p tokens);
token_p dict_parse(pres_p pres, token_p tokens);
token_p set_parse(pres_p pres, token_p tokens, pos_p poss);
token_p var_parse(pres_p pres, token_p tokens);
token_p func_def_parse(pres_p pres, token_p tokens);
token_p func_call_parse(pres_p pres, token_p tokens);
token_p class_def_parse(pres_p pres, token_p tokens);
token_p struct_def_parse(pres_p pres, token_p tokens);

token_p properties_gen(char* properties, token_p tokens);
token_p body_gen(body_p body, unsigned long long size, pres_p pres, token_p tokens);

pres_t parse(token_p tokens)
{
    pres_t pres;
    pres.has_error = 0;

    token_p tokens_copy = tokens;

    pres.nodes = malloc(NODES_SIZE * sizeof(node_t));

    unsigned long long alloc = NODES_SIZE;
    unsigned long long size = 0;
    do
    {
        for (; tokens->type == NEWLINE_T || tokens->type == SEMICOLON_T; tokens++);
        if (tokens->type == EOF_T)
            break;

        if (size == alloc)
            pres.nodes = realloc(pres.nodes, (alloc += NODES_SIZE) * sizeof(node_t));

        tokens = dollar_func(&pres, tokens);
        if (pres.has_error)
        {
            pres.nodes -= size;
            free(pres.nodes);
            free(tokens_copy);
            return pres;
        }

        size++;
        pres.nodes++;
    } while ((tokens - 1)->type == NEWLINE_T || tokens->type == SEMICOLON_T);

    if (tokens->type != EOF_T)
    {
        pres.nodes -= size;
        free(pres.nodes);
        free(tokens_copy);

        invalid_syntax_t error = invalid_syntax_set("Expected EOF", &tokens->poss, &tokens->pose);
        pres_fail(&pres, &error);
        return pres;
    }

    pres.nodes -= size;
    if (size + 1 != alloc)
        pres.nodes = realloc(pres.nodes, (size + 1) * sizeof(node_t));

    pres.nodes[size] = node_set3(NULL_N, &tokens->poss, &tokens->pose);

    free(tokens_copy);
    return pres;
}

void pres_fail(pres_p pres, invalid_syntax_p error)
{
    pres->error = *error;
    pres->has_error = 1;
}

token_p dollar_func(pres_p pres, token_p tokens)
{
    if (tokens->type == DOLLAR_T)
    {
        pos_p poss = &tokens++->poss;

        advance_newline(tokens);

        if (tokens->type != IDENTIFIER_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected dollar-function name", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        char* name = tokens->value;

        pos_p pose = &tokens++->pose;

        advance_newline(tokens);

        if (tokens->type != COLON_T)
        {
            dollar_func_call_np node = dollar_func_call_n_set(name, NULL, 0);
            *pres->nodes = node_set1(DOLLAR_FUNC_CALL_N, node, poss, pose);
            return tokens;
        }

        tokens++;
        advance_newline(tokens);

        tokens = assign(pres, tokens);
        if (pres->has_error)
            return tokens;

        node_p args = heap_alloc(&memory.heap, DOLLAR_FUNC_ARGS_SIZE * sizeof(node_t));
        *args = *pres->nodes;

        unsigned long long alloc = DOLLAR_FUNC_ARGS_SIZE;
        unsigned long long size = 1;

        pose = &args->pose;

        while (tokens->type == COMMA_T)
        {
            tokens++;
            advance_newline(tokens);

            tokens = assign(pres, tokens);
            if (pres->has_error)
                return tokens;

            if (size == alloc)
                args = heap_expand(&memory.heap, args, size * sizeof(node_t), (alloc += DOLLAR_FUNC_ARGS_SIZE) * sizeof(node_t));

            args[size++] = *pres->nodes;

            pose = &pres->nodes->pose;
        }

        if (size != alloc)
            heap_shrink(&memory.heap, args, alloc * sizeof(node_t), size * sizeof(node_t));

        dollar_func_call_np node = dollar_func_call_n_set(name, args, size);
        *pres->nodes = node_set1(DOLLAR_FUNC_CALL_N, node, poss, pose);
        return tokens;
    }

    return statement(pres, tokens);
}

token_p statement(pres_p pres, token_p tokens)
{
    if (tokens->type == RETURN_TK)
    {
        pos_p poss = &tokens->poss;
        pos_p pose = &tokens++->pose;

        advance_newline(tokens);

        token_p new = tuple(pres, tokens);
        if (pres->has_error)
        {
            pres->has_error = 0;

            *pres->nodes = node_set1(RETURN_N, NULL, poss, pose);
            return tokens;
        }

        return_np node = return_n_set(pres->nodes);
        *pres->nodes = node_set1(RETURN_N, node, poss, &pres->nodes->pose);
        return new;
    }

    if (tokens->type == CONTINUE_TK)
    {
        *pres->nodes = node_set3(CONTINUE_N, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }
    if (tokens->type == BREAK_TK)
    {
        *pres->nodes = node_set3(BREAK_N, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    return tuple(pres, tokens);
}

token_p tuple(pres_p pres, token_p tokens)
{
    tokens = assign(pres, tokens);
    if (pres->has_error)
        return tokens;

    if (tokens->type == COMMA_T)
    {
        pos_p pose;

        node_p elements = heap_alloc(&memory.heap, TUPLE_SIZE);
        *elements = *pres->nodes;

        unsigned long long alloc = TUPLE_SIZE;
        unsigned long long size = 1;

        token_p new;
        do
        {
            pose = &tokens++->pose;

            advance_newline(tokens);

            new = assign(pres, tokens);
            if (pres->has_error)
            {
                pres->has_error = 0;

                if (size != alloc)
                    heap_shrink(&memory.heap, elements, alloc * sizeof(node_t), size * sizeof(node_t));

                tuple_np node = tuple_n_set(elements, size);
                *pres->nodes = node_set1(TUPLE_N, node, &elements->poss, pose);
                return tokens;
            }
            tokens = new;

            if (size == alloc)
                elements = heap_expand(&memory.heap, elements, size * sizeof(node_t), (alloc += TUPLE_SIZE) * sizeof(node_t));

            elements[size++] = *pres->nodes;
        } while (tokens->type == COMMA_T);

        if (size != alloc)
            heap_shrink(&memory.heap, elements, alloc * sizeof(node_t), size * sizeof(node_t));

        tuple_np node = tuple_n_set(elements, size);
        *pres->nodes = node_set1(TUPLE_N, node, &elements->poss, &elements[size - 1].pose);
        return tokens;
    }

    return tokens;
}

token_p assign(pres_p pres, token_p tokens)
{
    if (tokens->type == VAR_TK)
        return var_parse(pres, tokens);

    tokens = ternary(pres, tokens);
    if (pres->has_error)
        return tokens;

    if (tokens->type >= ASSIGN_T && tokens->type <= RSHIFT_EQ_T)
    {
        unsigned char operator = tokens++->type;

        advance_newline(tokens);

        node_t var = *pres->nodes;

        tokens = tuple(pres, tokens);
        if (pres->has_error)
            return tokens;

        var_reassign_np node = var_reassign_n_set(operator, &var, pres->nodes);
        *pres->nodes = node_set1(VAR_REASSIGN_N, node, &var.poss, &pres->nodes->pose);
        return tokens;
    }

    return tokens;
}

token_p ternary(pres_p pres, token_p tokens)
{
    tokens = typeof(pres, tokens);
    if (pres->has_error)
        return tokens;

    if (tokens->type == QUESTION_T)
    {
        tokens++;
        advance_newline(tokens);

        node_t condition = *pres->nodes;

        node_t left;
        node_t right;

        if (tokens->type == COLON_T)
        {
            tokens++;
            advance_newline(tokens);

            tokens = dollar_func(pres, tokens);
            if (pres->has_error)
                return tokens;

            left.type = NULL_N;
            right = *pres->nodes;
        }
        else
        {
            tokens = dollar_func(pres, tokens);
            if (pres->has_error)
                return tokens;

            left = *pres->nodes;

            if (tokens->type == COLON_T)
            {
                tokens++;
                advance_newline(tokens);

                tokens = dollar_func(pres, tokens);
                if (pres->has_error)
                    return tokens;

                right = *pres->nodes;
            }
            else
                right.type = NULL_N;
        }

        ternary_condition_np node = ternary_condition_n_set(&condition, &left, &right);
        *pres->nodes = node_set1(TERNARY_CONDITION_N, node, &condition.poss, &pres->nodes->pose);
        return tokens;
    }

    return tokens;
}

token_p typeof(pres_p pres, token_p tokens)
{
    tokens = contain(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    unsigned char operator;
    while (tokens->type == IS_TK || tokens->type == ARE_TK)
    {
        operator = tokens++->type;

        advance_newline(tokens);

        tokens = contain(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(operator, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p contain(pres_p pres, token_p tokens)
{
    tokens = or(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == IN_TK)
    {
        tokens++;
        advance_newline(tokens);

        tokens = or(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(IN_TK, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p or(pres_p pres, token_p tokens)
{
    tokens = xor(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == OR_T || tokens->type == OR_TK)
    {
        tokens++;
        advance_newline(tokens);

        tokens = xor(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(OR_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p xor(pres_p pres, token_p tokens)
{
    tokens = and(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == XOR_T || tokens->type == XOR_TK)
    {
        tokens++;
        advance_newline(tokens);

        tokens = and(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(XOR_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p and(pres_p pres, token_p tokens)
{
    tokens = compare1(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == AND_T || tokens->type == AND_TK)
    {
        tokens++;
        advance_newline(tokens);

        tokens = compare1(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(AND_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p compare1(pres_p pres, token_p tokens)
{
    tokens = compare2(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    unsigned char operator;
    while (tokens->type == EQUAL_T || tokens->type == NEQUAL_T)
    {
        operator = tokens++->type;

        advance_newline(tokens);

        tokens = compare2(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(operator, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p compare2(pres_p pres, token_p tokens)
{
    tokens = b_or(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    unsigned char operator;
    while (tokens->type >= LESS_T && tokens->type <= GREATER_EQ_T)
    {
        operator = tokens++->type;

        advance_newline(tokens);

        tokens = b_or(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(operator, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p b_or(pres_p pres, token_p tokens)
{
    tokens = b_xor(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == B_OR_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = b_xor(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(B_OR_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p b_xor(pres_p pres, token_p tokens)
{
    tokens = b_and(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == B_XOR_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = b_and(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(B_XOR_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p b_and(pres_p pres, token_p tokens)
{
    tokens = shift(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == B_AND_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = shift(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(B_AND_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p shift(pres_p pres, token_p tokens)
{
    tokens = expression(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    unsigned char operator;
    while (tokens->type == LSHIFT_T || tokens->type == RSHIFT_T)
    {
        operator = tokens++->type;

        advance_newline(tokens);

        tokens = expression(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(operator, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p expression(pres_p pres, token_p tokens)
{
    tokens = term(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    unsigned char operator;
    while (tokens->type == PLUS_T || tokens->type == MINUS_T)
    {
        operator = tokens++->type;

        advance_newline(tokens);

        tokens = term(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(operator, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p term(pres_p pres, token_p tokens)
{
    tokens = factor(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    unsigned char operator;
    while (tokens->type >= MULTIPLY_T && tokens->type <= QUOTIENT_T)
    {
        operator = tokens++->type;

        advance_newline(tokens);

        tokens = factor(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(operator, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p factor(pres_p pres, token_p tokens)
{
    if (tokens->type == PLUS_T || tokens->type == MINUS_T ||
        tokens->type == B_NOT_T ||
        tokens->type == NOT_T || tokens->type == NOT_TK)
    {
        pos_p poss = &tokens->poss;

        unsigned char operator = tokens++->type;

        advance_newline(tokens);

        tokens = factor(pres, tokens);
        if (pres->has_error)
            return tokens;

        unary_operation_np node = unary_operation_n_set(operator, pres->nodes);
        *pres->nodes = node_set1(UNARY_OPERATION_N, node, poss, &pres->nodes->pose);
        return tokens;
    }

    if (tokens->type == INCREMENT_T || tokens->type == DECREMENT_T)
    {
        pos_p poss = &tokens->poss;

        unsigned char operator = tokens++->type;

        advance_newline(tokens);

        tokens = factor(pres, tokens);
        if (pres->has_error)
            return tokens;

        var_fixed_assign_np node = var_fixed_assign_n_set(VFA_PROP(0), operator, pres->nodes);
        *pres->nodes = node_set1(VAR_FIXED_ASSIGN_N, node, poss, &pres->nodes->pose);
        return tokens;
    }

    return exponent(pres, tokens);
}

token_p exponent(pres_p pres, token_p tokens)
{
    tokens = post(pres, tokens);
    if (pres->has_error)
        return tokens;

    node_t left = *pres->nodes;

    while (tokens->type == POWER_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = factor(pres, tokens);
        if (pres->has_error)
            return tokens;

        left.value.ptr = binary_operation_n_set(POWER_T, &left, pres->nodes);
        left.type = BINARY_OPERATION_N;
        left.pose = pres->nodes->pose;
    }

    *pres->nodes = left;
    return tokens;
}

token_p post(pres_p pres, token_p tokens)
{
    tokens = core(pres, tokens);
    if (pres->has_error)
        return tokens;

check:
    if (tokens->type == LSQUARE_T)
    {
        tokens++;
        advance_newline(tokens);

        node_t value = *pres->nodes;

        tokens = tuple(pres, tokens);
        if (pres->has_error)
            return tokens;

        if (tokens->type != RSQUARE_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected ']'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        tokens++;
        advance_newline(tokens);

        subscript_np node = subscript_n_set(&value, pres->nodes);
        *pres->nodes = node_set1(SUBSCRIPT_N, node, &value.poss, &pres->nodes->pose);
        goto check;
    }

    if (tokens->type == DOT_T)
    {
        tokens++;
        advance_newline(tokens);

        node_t value = *pres->nodes;

        tokens = core(pres, tokens);
        if (pres->has_error)
            return tokens;

        access_np node = access_n_set(&value, pres->nodes);
        *pres->nodes = node_set1(ACCESS_N, node, &value.poss, &pres->nodes->pose);
        goto check;
    }

    if (tokens->type == INCREMENT_T || tokens->type == DECREMENT_T)
    {
        unsigned char operator = tokens->type;

        pos_p poss = &tokens++->poss;

        advance_newline(tokens);

        var_fixed_assign_np node = var_fixed_assign_n_set(VFA_PROP(1), operator, pres->nodes);
        *pres->nodes = node_set1(VAR_FIXED_ASSIGN_N, node, poss, &pres->nodes->pose);
        goto check;
    }

    if (tokens->type == LPAREN_T)
    {
        tokens = func_call_parse(pres, tokens);
        if (pres->has_error)
            return tokens;

        goto check;
    }

    return tokens;
}

token_p core(pres_p pres, token_p tokens)
{
    if (tokens->type == LPAREN_T)
    {
        pos_p poss = &tokens++->poss;

        advance_newline(tokens);

        tokens = dollar_func(pres, tokens);
        if (pres->has_error)
            return tokens;

        if (tokens->type != RPAREN_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected ')'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        pres->nodes->poss = *poss;
        pres->nodes->pose = tokens++->pose;

        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == NONE_TK)
    {
        *pres->nodes = node_set3(NONE_N, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == INT_T)
    {
        int_np node = int_n_set(tokens->value, tokens->size);
        *pres->nodes = node_set1(INT_N, node, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }
    if (tokens->type == FLOAT_T)
    {
        float_np node = float_n_set(tokens->value, tokens->size);
        *pres->nodes = node_set1(FLOAT_N, node, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }
    if (tokens->type == COMPLEX_T)
    {
        complex_np node = complex_n_set(tokens->value, tokens->size);
        *pres->nodes = node_set1(COMPLEX_N, node, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == TRUE_TK)
    {
        *pres->nodes = node_set2(BOOL_N, 1, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }
    if (tokens->type == FALSE_TK)
    {
        *pres->nodes = node_set2(BOOL_N, 0, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == CHAR_T)
    {
        *pres->nodes = node_set2(CHAR_N, (char)tokens->size, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == STR_T)
    {
        str_np node = str_n_set(tokens->value, tokens->size);
        *pres->nodes = node_set1(STR_N, node, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == LSQUARE_T)
        return list_parse(pres, tokens);

    if (tokens->type == LCURLY_T)
        return dict_parse(pres, tokens);

    if (tokens->type >= OBJECT_TT && tokens->type <= SET_TT)
    {
        *pres->nodes = node_set2(TYPE_N, tokens->type, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == IDENTIFIER_T)
    {
        *pres->nodes = node_set1(VAR_ACCESS_N, tokens->value, &tokens->poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    if (tokens->type == FUNC_TK)
        return func_def_parse(pres, tokens);

    if (tokens->type == CLASS_TK)
        return class_def_parse(pres, tokens);

    if (tokens->type == IMPORT_TK)
    {
        pos_p poss = &tokens++->poss;

        advance_newline(tokens);

        if (tokens->type != IDENTIFIER_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected identifier", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        *pres->nodes = node_set1(IMPORT_N, tokens->value, poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }
    if (tokens->type == INCLUDE_TK)
    {
        pos_p poss = &tokens++->poss;

        advance_newline(tokens);

        if (tokens->type != IDENTIFIER_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected identifier", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        *pres->nodes = node_set1(INCLUDE_N, tokens->value, poss, &tokens->pose);

        tokens++;
        advance_newline(tokens);

        return tokens;
    }

    invalid_syntax_t error = invalid_syntax_set(NULL, &tokens->poss, &tokens->pose);
    pres_fail(pres, &error);
    return tokens;
}

token_p list_parse(pres_p pres, token_p tokens)
{
    pos_p poss = &tokens++->poss;

    advance_newline(tokens);

    if (tokens->type == RSQUARE_T)
    {
        *pres->nodes = node_set1(LIST_N, NULL, poss, &tokens++->pose);

        advance_newline(tokens);

        return tokens;
    }

    node_p elements = heap_alloc(&memory.heap, LIST_SIZE * sizeof(node_t));

    unsigned long long alloc = LIST_SIZE;
    unsigned long long size = 0;

    do
    {
        tokens = assign(pres, tokens);
        if (pres->has_error)
            return tokens;

        if (size == alloc)
            elements = heap_expand(&memory.heap, elements, size * sizeof(node_t), (alloc += LIST_SIZE) * sizeof(node_t));

        elements[size++] = *pres->nodes;

        if (tokens->type != COMMA_T)
            break;

        tokens++;
        advance_newline(tokens);
    } while (tokens->type != RSQUARE_T);

    if (tokens->type != RSQUARE_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected ']'", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    if (size != alloc)
        heap_shrink(&memory.heap, elements, alloc * sizeof(node_t), size * sizeof(node_t));

    list_np node = list_n_set(elements, size);
    *pres->nodes = node_set1(LIST_N, node, poss, &tokens++->pose);

    advance_newline(tokens);

    return tokens;
}

token_p dict_parse(pres_p pres, token_p tokens)
{
    pos_p poss = &tokens++->poss;

    advance_newline(tokens);

    if (tokens->type == RCURLY_T)
    {
        *pres->nodes = node_set1(DICT_N, NULL, poss, &tokens++->pose);

        advance_newline(tokens);

        return tokens;
    }

    tokens = assign(pres, tokens);
    if (pres->has_error)
        return tokens;

    if (tokens->type == COMMA_T)
    {
        tokens++;
        advance_newline(tokens);

        return set_parse(pres, tokens, poss);
    }
    if (tokens->type == RCURLY_T)
    {
        node_p elements = heap_alloc(&memory.heap, sizeof(node_t));
        *elements = *pres->nodes;

        set_np node = set_n_set(elements, 1);
        *pres->nodes = node_set1(SET_N, node, poss, &tokens++->pose);

        advance_newline(tokens);

        return tokens;
    }

    pair_p elements = heap_alloc(&memory.heap, DICT_SIZE * sizeof(pair_t));
    elements->key = *pres->nodes;
    
    if (tokens->type != COLON_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected ':'", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    tokens++;
    advance_newline(tokens);

    tokens = assign(pres, tokens);
    if (pres->has_error)
        return tokens;

    elements->value = *pres->nodes;

    unsigned long long alloc = DICT_SIZE;
    unsigned long long size = 1;

    if (tokens->type == COMMA_T)
    {
        tokens++;
        advance_newline(tokens);

        while (tokens->type != RCURLY_T)
        {
            tokens = assign(pres, tokens);
            if (pres->has_error)
                return tokens;

            if (size == alloc)
                elements = heap_expand(&memory.heap, elements, size * sizeof(pair_t), (alloc += DICT_SIZE) * sizeof(pair_t));

            elements[size].key = *pres->nodes;

            if (tokens->type != COLON_T)
            {
                invalid_syntax_t error = invalid_syntax_set("Expected ':'", &tokens->poss, &tokens->pose);
                pres_fail(pres, &error);
                return tokens;
            }

            tokens++;
            advance_newline(tokens);

            tokens = assign(pres, tokens);
            if (pres->has_error)
                return tokens;

            elements[size++].value = *pres->nodes;

            if (tokens->type != COMMA_T)
                break;

            tokens++;
            advance_newline(tokens);
        }
    }

    if (tokens->type != RCURLY_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected '}'", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    if (size != alloc)
        heap_shrink(&memory.heap, elements, alloc * sizeof(pair_t), size * sizeof(pair_t));

    dict_np node = dict_n_set(elements, size);
    *pres->nodes = node_set1(DICT_N, node, poss, &tokens++->pose);

    advance_newline(tokens);

    return tokens;
}

token_p set_parse(pres_p pres, token_p tokens, pos_p poss)
{
    node_p elements = heap_alloc(&memory.heap, SET_SIZE * sizeof(node_t));
    *elements = *pres->nodes;

    unsigned long long alloc = SET_SIZE;
    unsigned long long size = 1;

    while (tokens->type != RCURLY_T)
    {
        tokens = assign(pres, tokens);
        if (pres->has_error)
            return tokens;

        if (size == alloc)
            elements = heap_expand(&memory.heap, elements, size * sizeof(node_t), (alloc += SET_SIZE) * sizeof(node_t));

        elements[size++] = *pres->nodes;

        if (tokens->type != COMMA_T)
            break;

        tokens++;
        advance_newline(tokens);
    }

    if (tokens->type != RCURLY_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected '}'", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    if (size != alloc)
        heap_shrink(&memory.heap, elements, alloc * sizeof(node_t), size * sizeof(node_t));

    set_np node = set_n_set(elements, size);
    *pres->nodes = node_set1(SET_N, node, poss, &tokens->pose);

    tokens++;
    advance_newline(tokens);

    return tokens;
}

token_p var_parse(pres_p pres, token_p tokens)
{
    pos_p poss = &tokens++->poss;

    advance_newline(tokens);

    char properties = 0;
    tokens = properties_gen(&properties, tokens);

    unsigned char type;
    if (tokens->type >= OBJECT_TT && tokens->type <= SET_TT)
    {
        type = tokens++->type;

        advance_newline(tokens);
    }
    else
        type = 0;

    if (tokens->type != IDENTIFIER_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected identifier", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    char* name = tokens++->value;

    advance_newline(tokens);

    if (tokens->type != ASSIGN_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected '='", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    tokens++;
    advance_newline(tokens);

    tokens = tuple(pres, tokens);
    if (pres->has_error)
        return tokens;

    var_assign_np node = var_assign_n_set(properties, name, type, pres->nodes);
    *pres->nodes = node_set1(VAR_ASSIGN_N, node, poss, &pres->nodes->pose);
    return tokens;
}

token_p func_def_parse(pres_p pres, token_p tokens)
{
    pos_p poss = &tokens++->poss;

    advance_newline(tokens);

    char properties = 0;
    tokens = properties_gen(&properties, tokens);

    unsigned char type;
    if (tokens->type >= OBJECT_TT && tokens->type <= SET_TT)
    {
        type = tokens++->type;

        advance_newline(tokens);
    }
    else
        type = 0;

    char* name;
    if (tokens->type == IDENTIFIER_T)
    {
        name = tokens++->value;

        advance_newline(tokens);
    }
    else
        name = NULL;

    if (tokens->type != LPAREN_T)
    {
        invalid_syntax_t error = invalid_syntax_set("Expected '('", &tokens->poss, &tokens->pose);
        pres_fail(pres, &error);
        return tokens;
    }

    tokens++;
    advance_newline(tokens);

    arg_p args;

    unsigned long long size = 0;

    if (tokens->type != RPAREN_T)
    {
        args = heap_alloc(&memory.heap, FUNC_DEF_SIZE * sizeof(arg_t));

        unsigned long long alloc = FUNC_DEF_SIZE;

        do
        {
            if (size == alloc)
                args = heap_expand(&memory.heap, args, size * sizeof(arg_t), (alloc += FUNC_DEF_SIZE) * sizeof(arg_t));

            if (tokens->type >= OBJECT_TT && tokens->type <= SET_TT)
            {
                args[size].type = tokens++->type;

                advance_newline(tokens);
            }
            else
                args[size].type = 0;

            if (tokens->type != IDENTIFIER_T)
            {
                invalid_syntax_t error = invalid_syntax_set("Expected identifier", &tokens->poss, &tokens->pose);
                pres_fail(pres, &error);
                return tokens;
            }

            args[size].name = tokens++->value;

            advance_newline(tokens);

            if (tokens->type == ASSIGN_T)
            {
                tokens++;
                advance_newline(tokens);

                tokens = assign(pres, tokens);
                if (pres->has_error)
                    return tokens;

                args[size++].value = *pres->nodes;
            }
            else
                args[size++].value.type = NONE_N;

            if (tokens->type != COMMA_T)
                break;

            tokens++;
            advance_newline(tokens);
        } while (1);

        if (tokens->type != RPAREN_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected ')'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        if (size != alloc)
            heap_shrink(&memory.heap, args, alloc * sizeof(arg_t), size * sizeof(arg_t));
    }

    tokens++;
    advance_newline(tokens);

    body_t body;

    pos_p pose;

    if (tokens->type == COLON_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = dollar_func(pres, tokens);
        if (pres->has_error)
            return tokens;

        body = body_set(pres->nodes);

        pose = &pres->nodes->pose;
    }
    else
    {
        if (tokens->type != LCURLY_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected '{' or ':'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        tokens++;
        advance_newline(tokens);

        if (tokens->type == RCURLY_T)
            body.size = 0;
        else
        {
            tokens = body_gen(&body, FUNC_DEF_BODY_SIZE, pres, tokens);
            if (pres->has_error)
                return tokens;

            if (tokens->type != RCURLY_T)
            {
                invalid_syntax_t error = invalid_syntax_set("Expected '}'", &tokens->poss, &tokens->pose);
                pres_fail(pres, &error);
                return tokens;
            }
        }

        pose = &tokens++->pose;

        advance_newline(tokens);
    }

    func_def_np node = func_def_n_set(properties, name, args, size, type, &body);
    *pres->nodes = node_set1(FUNC_DEF_N, node, poss, pose);
    return tokens;
}

token_p func_call_parse(pres_p pres, token_p tokens)
{
    tokens++;
    advance_newline(tokens);

    node_t func = *pres->nodes;

    if (tokens->type == RPAREN_T)
    {
        func_call_np node = func_call_n_set(&func, NULL, 0);
        *pres->nodes = node_set1(FUNC_CALL_N, node, &func.poss, &tokens++->pose);
    }
    else
    {
        arg_access_p args = heap_alloc(&memory.heap, FUNC_CALL_SIZE * sizeof(arg_access_t));

        unsigned long long alloc = FUNC_CALL_SIZE;
        unsigned long long size = 0;

        do
        {
            if (size == alloc)
                args = heap_expand(&memory.heap, args, size * sizeof(arg_access_t), (alloc += FUNC_CALL_SIZE) * sizeof(arg_access_t));

            if (tokens->type == IDENTIFIER_T)
            {
                if ((tokens + 1)->type == COLON_T)
                {
                    args[size].name = tokens++->value;

                    tokens++;
                    advance_newline(tokens);
                }
                else if ((tokens + 1)->type == NEWLINE_T && (tokens + 2)->type == COLON_T)
                {
                    args[size].name = tokens++->value;

                    tokens += 2;
                    advance_newline(tokens);
                }
                else
                    args[size].name = NULL;
            }
            else
                args[size].name = NULL;

            tokens = assign(pres, tokens);
            if (pres->has_error)
                return tokens;

            args[size++].value = *pres->nodes;

            if (tokens->type != COMMA_T)
                break;

            tokens++;
            advance_newline(tokens);
        } while (1);

        if (tokens->type != RPAREN_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected ')'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        if (size != alloc)
            heap_shrink(&memory.heap, args, alloc * sizeof(arg_access_t), size * sizeof(arg_access_t));

        func_call_np node = func_call_n_set(&func, args, size);
        *pres->nodes = node_set1(FUNC_CALL_N, node, &func.poss, &tokens++->pose);
    }

    advance_newline(tokens);
    return tokens;
}

token_p class_def_parse(pres_p pres, token_p tokens)
{
    pos_p poss = &tokens++->poss;

    advance_newline(tokens);

    char properties = 0;
    tokens = properties_gen(&properties, tokens);

    char* name;
    if (tokens->type == IDENTIFIER_T)
    {
        name = tokens++->value;

        advance_newline(tokens);
    }
    else
        name = NULL;

    body_t body;

    pos_p pose;

    if (tokens->type == COLON_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = dollar_func(pres, tokens);
        if (pres->has_error)
            return tokens;

        body = body_set(pres->nodes);

        pose = &pres->nodes->pose;
    }
    else
    {
        if (tokens->type != LCURLY_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected '{' or ':'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        tokens++;
        advance_newline(tokens);

        if (tokens->type == RCURLY_T)
            body.size = 0;
        else
        {
            tokens = body_gen(&body, CLASS_DEF_BODY_SIZE, pres, tokens);
            if (pres->has_error)
                return tokens;

            if (tokens->type != RCURLY_T)
            {
                invalid_syntax_t error = invalid_syntax_set("Expected '}'", &tokens->poss, &tokens->pose);
                pres_fail(pres, &error);
                return tokens;
            }
        }

        pose = &tokens++->pose;

        advance_newline(tokens);
    }

    class_def_np node = class_def_n_set(properties, name, &body);
    *pres->nodes = node_set1(CLASS_DEF_N, node, poss, pose);
    return tokens;
}

token_p struct_def_parse(pres_p pres, token_p tokens)
{
    pos_p poss = &tokens++->poss;

    advance_newline(tokens);

    char properties = 0;
    tokens = properties_gen(&properties, tokens);

    char* name;
    if (tokens->type == IDENTIFIER_T)
    {
        name = tokens++->value;

        advance_newline(tokens);
    }
    else
        name = NULL;

    body_t body;

    pos_p pose;

    if (tokens->type == COLON_T)
    {
        tokens++;
        advance_newline(tokens);

        tokens = dollar_func(pres, tokens);
        if (pres->has_error)
            return tokens;

        body = body_set(pres->nodes);

        pose = &pres->nodes->pose;
    }
    else
    {
        if (tokens->type != LCURLY_T)
        {
            invalid_syntax_t error = invalid_syntax_set("Expected '{' or ':'", &tokens->poss, &tokens->pose);
            pres_fail(pres, &error);
            return tokens;
        }

        tokens++;
        advance_newline(tokens);

        if (tokens->type == RCURLY_T)
            body.size = 0;
        else
        {
            tokens = body_gen(&body, STRUCT_DEF_BODY_SIZE, pres, tokens);
            if (pres->has_error)
                return tokens;

            if (tokens->type != RCURLY_T)
            {
                invalid_syntax_t error = invalid_syntax_set("Expected '}'", &tokens->poss, &tokens->pose);
                pres_fail(pres, &error);
                return tokens;
            }
        }

        pose = &tokens++->pose;

        advance_newline(tokens);
    }

    struct_def_np node = struct_def_n_set(properties, name, &body);
    *pres->nodes = node_set1(STRUCT_DEF_N, node, poss, pose);
    return tokens;
}

token_p properties_gen(char* properties, token_p tokens)
{
    char public = 0;
    char global = 0;

    while (1)
    {
        if (!public && tokens->type == PUBLIC_TK)
        {
            public = 1;
            *properties |= 1;

            tokens++;
            advance_newline(tokens);
            continue;
        }
        if (!public && tokens->type == PRIVATE_TK)
        {
            public = 1;

            tokens++;
            advance_newline(tokens);
            continue;
        }

        if (!global && tokens->type == GLOBAL_TK)
        {
            global = 1;
            *properties |= 2;

            tokens++;
            advance_newline(tokens);
            continue;
        }
        if (!global && tokens->type == LOCAL_TK)
        {
            global = 1;

            tokens++;
            advance_newline(tokens);
            continue;
        }

        if (!PROP_CONST(*properties) && tokens->type == CONST_TK)
        {
            *properties |= 4;

            tokens++;
            advance_newline(tokens);
            continue;
        }

        if (!PROP_STATIC(*properties) && tokens->type == STATIC_TK)
        {
            *properties |= 8;

            tokens++;
            advance_newline(tokens);
            continue;
        }

        break;
    }

    return tokens;
}

token_p body_gen(body_p body, unsigned long long size, pres_p pres, token_p tokens)
{
    body->nodes = heap_alloc(&memory.heap, size * sizeof(node_t));

    unsigned long long alloc = size;
    body->size = 0;

    do
    {
        for (; tokens->type == NEWLINE_T || tokens->type == SEMICOLON_T; tokens++);
        if (tokens->type == RCURLY_T || tokens->type == EOF_T)
            break;

        if (body->size == alloc)
            body->nodes = heap_expand(&memory.heap, body->nodes, body->size * sizeof(node_t), (alloc += size) * sizeof(node_t));

        tokens = dollar_func(pres, tokens);
        if (pres->has_error)
            return tokens;

        body->nodes[body->size++] = *pres->nodes;
    } while ((tokens - 1)->type == NEWLINE_T || tokens->type == SEMICOLON_T);

    return tokens;
}
