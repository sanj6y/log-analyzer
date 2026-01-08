# Testing Guide for Part 2: Aggregator Engine

## Basic Tests

### 1. Basic Log Level Frequency Analysis
```bash
./build/log-analyzer data/sample.log
```
**Expected Output:**
- Processing summary with throughput
- Log level frequency histogram showing counts and percentages for ERROR, WARNING, INFO, DEBUG, TRACE

### 2. Time Window Analysis
```bash
./build/log-analyzer data/sample.log --show-windows
```
**Expected Output:**
- All basic output plus time window breakdown
- Shows entries per time window (default 60 seconds)
- Error/Warning/Info counts per window

### 3. Custom Time Window Size
```bash
./build/log-analyzer data/sample.log --show-windows --time-window 30
```
**Expected Output:**
- Same as above but with 30-second windows instead of 60

### 4. Pattern Matching
```bash
./build/log-analyzer data/sample.log --pattern "ERROR" --pattern "timeout" --pattern "Connection"
```
**Expected Output:**
- All basic output plus pattern match counts at the end

### 5. Combined Features
```bash
./build/log-analyzer data/sample.log --show-windows --time-window 30 --pattern "ERROR" --pattern "timeout"
```
**Expected Output:**
- Complete analysis with histogram, time windows, and pattern matches

## Specific Test Cases

### Test 1: Verify Log Level Detection
```bash
# Should show all 5 log levels with roughly equal distribution
./build/log-analyzer data/sample.log | grep -A 10 "Log Level Frequency"
```

### Test 2: Verify Time Window Boundaries
```bash
# 30-second windows - should create many windows for ~83 minutes of logs
./build/log-analyzer data/sample.log --show-windows --time-window 30 | grep "Window Start" | wc -l
# Should show ~166 windows (83 minutes / 0.5 minutes per window)
```

### Test 3: Verify Pattern Matching
```bash
# Count ERROR occurrences
./build/log-analyzer data/sample.log --pattern "ERROR" | grep "ERROR.*matches"
# Should match the ERROR count in the histogram
```

### Test 4: Large Buffer Test
```bash
# Test with larger buffer
./build/log-analyzer data/sample.log --buffer-size 2097152
# Should process faster (larger buffer = fewer I/O operations)
```

### Test 5: Verify Zero-Copy Performance
```bash
# Measure processing time
time ./build/log-analyzer data/sample.log
# Should be very fast (<1 second for 50k lines)
```

## Advanced Tests

### Test Error Spike Detection
```bash
# Look for time windows with high error rates
./build/log-analyzer data/sample.log --show-windows --time-window 10 | grep -E "Window Start|ERROR" | head -40
```

### Test Pattern Frequency
```bash
# Find most common patterns
./build/log-analyzer data/sample.log \
  --pattern "ERROR" \
  --pattern "WARNING" \
  --pattern "timeout" \
  --pattern "Connection" \
  --pattern "File not found" \
  | grep "Pattern Matches" -A 10
```

## Performance Benchmarks

### Baseline Test
```bash
time ./build/log-analyzer data/sample.log
```

### With All Features
```bash
time ./build/log-analyzer data/sample.log \
  --show-windows \
  --time-window 30 \
  --pattern "ERROR" \
  --pattern "WARNING"
```

## Expected Results

### For `data/sample.log` (50,000 lines):
- **Total lines:** ~50,000
- **File size:** ~2.7 MB
- **Processing time:** < 500ms
- **Throughput:** > 2 MB/min
- **Log levels:** Roughly equal distribution (20% each)
- **Time windows:** ~83 windows at 60-second intervals, ~166 at 30-second intervals

## Verification Checklist

- [ ] Log level histogram shows correct counts
- [ ] Percentages sum to ~100%
- [ ] Time windows are created correctly
- [ ] Time window counts match histogram totals
- [ ] Pattern matching finds occurrences
- [ ] Processing completes without errors
- [ ] Memory usage remains constant (check with `top` or `htop`)
- [ ] Throughput is consistent across runs

## Troubleshooting

### If pattern matching returns 0:
- Check that the pattern exists in the log file
- Patterns are case-sensitive
- Try simpler patterns first

### If time windows are empty:
- Verify timestamps are in format "YYYY-MM-DD HH:MM:SS"
- Check that log level extraction is working

### If performance is slow:
- Increase buffer size with `--buffer-size`
- Check system resources
- Verify release build: `cmake --build . --config Release`
