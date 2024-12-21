#include "Parser.h"

Parser::Parser() {
    parseTable[{Symbol::A, Symbol::id}] = {Symbol::id, Symbol::=, Symbol::E, Symbol::;, Symbol::@assign, };
    parseTable[{Symbol::D, Symbol::type}] = {Symbol::type, Symbol::id, Symbol::;, Symbol::@declare, };
    parseTable[{Symbol::E, Symbol::(}] = {Symbol::T, Symbol::E', };
    parseTable[{Symbol::E, Symbol::id}] = {Symbol::T, Symbol::E', };
    parseTable[{Symbol::E, Symbol::num}] = {Symbol::T, Symbol::E', };
    parseTable[{Symbol::E', Symbol::)}] = {Symbol::epsilon, };
    parseTable[{Symbol::E', Symbol::+}] = {Symbol::+, Symbol::T, Symbol::E', Symbol::@add, };
    parseTable[{Symbol::E', Symbol::-}] = {Symbol::-, Symbol::T, Symbol::E', Symbol::@sub, };
    parseTable[{Symbol::E', Symbol::;}] = {Symbol::epsilon, };
    parseTable[{Symbol::F, Symbol::(}] = {Symbol::(, Symbol::E, Symbol::), Symbol::@value, };
    parseTable[{Symbol::F, Symbol::id}] = {Symbol::id, Symbol::@value, };
    parseTable[{Symbol::F, Symbol::num}] = {Symbol::num, Symbol::@value, };
    parseTable[{Symbol::P, Symbol::id}] = {Symbol::S, };
    parseTable[{Symbol::P, Symbol::type}] = {Symbol::S, };
    parseTable[{Symbol::S, Symbol::id}] = {Symbol::A, };
    parseTable[{Symbol::S, Symbol::type}] = {Symbol::D, };
    parseTable[{Symbol::T, Symbol::(}] = {Symbol::F, Symbol::T', };
    parseTable[{Symbol::T, Symbol::id}] = {Symbol::F, Symbol::T', };
    parseTable[{Symbol::T, Symbol::num}] = {Symbol::F, Symbol::T', };
    parseTable[{Symbol::T', Symbol::)}] = {Symbol::epsilon, };
    parseTable[{Symbol::T', Symbol::*}] = {Symbol::*, Symbol::F, Symbol::T', Symbol::@mul, };
    parseTable[{Symbol::T', Symbol::+}] = {Symbol::epsilon, };
    parseTable[{Symbol::T', Symbol::-}] = {Symbol::epsilon, };
    parseTable[{Symbol::T', Symbol::/}] = {Symbol::/, Symbol::F, Symbol::T', Symbol::@div, };
    parseTable[{Symbol::T', Symbol::;}] = {Symbol::epsilon, };
}

bool Parser::parse(const vector<Symbol>& input) {
    stack<Symbol> stack;
    stack.push(Symbol::P);
    size_t inputPos = 0;

    while (!stack.empty() && inputPos <= input.size()) {
        if (stack.empty()) return inputPos >= input.size();
        Symbol top = stack.top();
        stack.pop();

        if (top == Symbol::epsilon) continue;

        if (inputPos >= input.size()) return false;
        Symbol current = input[inputPos];

        if (top == current) {
            inputPos++;
            continue;
        }

        auto it = parseTable.find({top, current});
        if (it == parseTable.end()) return false;

        const auto& production = it->second;
        for (auto it = production.rbegin(); it != production.rend(); ++it) {
            stack.push(*it);
        }
    }

    return stack.empty() && inputPos >= input.size();
}
