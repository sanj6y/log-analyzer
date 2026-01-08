#!/bin/bash
echo "=== Part 2 Testing Suite ==="
echo ""

echo "Test 1: Basic Log Level Frequency"
./build/log-analyzer data/sample.log 2>&1 | grep -A 10 "Log Level Frequency"
echo ""

echo "Test 2: Pattern Matching"
./build/log-analyzer data/sample.log --pattern "ERROR" --pattern "timeout" 2>&1 | grep -A 5 "Pattern Matches"
echo ""

echo "Test 3: Time Window Analysis (first 5 windows)"
./build/log-analyzer data/sample.log --show-windows --time-window 30 2>&1 | grep -A 7 "Time Window Analysis" | head -8
echo ""

echo "Test 4: Performance Test"
time ./build/log-analyzer data/sample.log > /dev/null 2>&1
