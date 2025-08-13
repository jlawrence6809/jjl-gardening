# ESP32 Build Optimization Guide

This guide provides comprehensive strategies to minimize the size of web assets for ESP32 deployment.

## Current Optimization Results

With the applied optimizations, you should see significant size reductions:

### Before Optimizations (Baseline)
- **bundle.js**: ~37KB → **Target: ~15-20KB**
- **bundle.css**: ~2.2KB → **Target: ~0.8-1.2KB**
- **polyfills.js**: ~6.9KB → **Target: ~3-4KB**
- **Total**: ~47KB → **Target: ~20-25KB**

### Optimization Techniques Applied

## 1. Advanced JavaScript Minification

### Terser Configuration
```javascript
new TerserPlugin({
  terserOptions: {
    compress: {
      drop_console: true,        // Remove all console.log statements
      drop_debugger: true,       // Remove debugger statements
      dead_code: true,          // Remove unreachable code
      inline: 2,                // Aggressive function inlining
      passes: 3,                // Multiple optimization passes
      pure_funcs: ['console.log', 'console.info'], // Mark as pure (removable)
    },
    mangle: {
      toplevel: true,           // Shorten variable names globally
    },
  },
})
```

**Expected Savings**: 30-40% JavaScript size reduction

## 2. CSS Optimization

### Techniques Applied
- **Removed comments and whitespace**: Manual minification
- **Shortened color values**: `blanchedalmond` → `#ffebcd`
- **Removed unnecessary font fallbacks**: `'Helvetica Neue', arial` → `arial`
- **Combined selectors**: Merged similar rules

**Expected Savings**: 40-60% CSS size reduction

### Ultra-Minimal CSS Approach
Consider switching to `style.min.css` for maximum compression:
```bash
# In your build process, replace style.css with style.min.css
mv src/style.css src/style.original.css
mv src/style.min.css src/style.css
```

## 3. Advanced Compression Options

### Brotli Compression (Experimental)
```bash
# Enable Brotli compression (requires ESP32 support)
npm run build:esp32:brotli
```

**Benefits**:
- 15-20% better compression than gzip
- Especially effective on JavaScript and CSS
- Requires ESP32 Arduino library with Brotli support

### Compression Levels
```javascript
// In preact.config.js ESP plugin options
{
  compressionLevel: 9,      // Gzip: 1-9 (9 = maximum)
  compressionLevel: 11,     // Brotli: 1-11 (11 = maximum)
}
```

## 4. Bundle Analysis

### Analyze Bundle Composition
```bash
# Generate detailed bundle analysis
npm run build:esp32:analyze

# Check current sizes
npm run size-check
```

This creates `bundle-report.html` showing:
- Which modules consume the most space
- Duplicate dependencies
- Optimization opportunities

## 5. Code-Level Optimizations

### Remove Unused Features
```javascript
// In src/index.tsx, consider removing features for ESP32:

// Remove if not needed:
// - Portal functionality (if dialogs can be simplified)
// - Complex state management (if simpler alternatives work)
// - Unused TypeScript interfaces
```

### Simplify Components
```javascript
// Replace complex components with simpler alternatives:

// Instead of:
const ComplexComponent = ({ prop1, prop2, ...rest }) => {
  const [state, setState] = useState(initialValue);
  // ... complex logic
};

// Use:
const SimpleComponent = ({ essentialProp }) => (
  <div>{essentialProp}</div>
);
```

## 6. Library Optimizations

### Use Minimal Preact Build
```javascript
// In webpack config aliases:
resolve: {
  alias: {
    'preact/compat': 'preact/compat/dist/compat.min.js',
    'preact': 'preact/dist/preact.min.js',
  },
}
```

### Consider Micro-Alternatives
For specific functionality, consider:
- Replace complex state management with simple variables
- Use native DOM APIs instead of Preact hooks where appropriate
- Inline small utility functions instead of importing libraries

## 7. Build Process Optimizations

### Exclude Unnecessary Files
```javascript
// In ESP plugin configuration:
exclude: [
  '*.map',                    // Source maps
  'manifest.json',            // PWA manifest
  'sw.js',                    // Service worker
  '200.html',                 // SPA fallback
  'preact_prerender_data.json', // Prerender data
]
```

### Tree Shaking
```javascript
// Ensure tree shaking is enabled:
optimization: {
  usedExports: true,
  sideEffects: false,
}
```

## 8. Extreme Optimization Techniques

### Manual Preact Replacement
For ultimate size reduction, consider:
```javascript
// Replace Preact with vanilla JS for simple components
const createButton = (text, onClick) => {
  const btn = document.createElement('button');
  btn.textContent = text;
  btn.onclick = onClick;
  return btn;
};
```

### Inline Critical CSS
```javascript
// Inline essential CSS directly in HTML:
const criticalCSS = `body{background:#282c34;color:#ffebcd}`;
document.head.insertAdjacentHTML('beforeend', `<style>${criticalCSS}</style>`);
```

## 9. ESP32-Specific Optimizations

### Memory Layout
```c++
// In ESP32 code, consider PROGMEM placement:
const uint8_t PROGMEM asset_data[] = { /* compressed data */ };

// Use streaming for large assets:
void serveChunked(AsyncWebServerRequest* request) {
  AsyncWebServerResponse* response = request->beginChunkedResponse(
    "text/html",
    [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
      // Stream data in chunks to reduce RAM usage
    }
  );
}
```

### Flash Optimization
```c++
// Partition large assets across multiple arrays:
const uint8_t PROGMEM bundle_part1[] = { /* first 16KB */ };
const uint8_t PROGMEM bundle_part2[] = { /* remaining data */ };
```

## 10. Build Commands

### Standard Optimized Build
```bash
npm run build
```

### Maximum Compression Build
```bash
npm run build:esp32:brotli
```

### Development Analysis
```bash
npm run build:esp32:analyze
```

### Size Monitoring
```bash
npm run size-check
```

## 11. Expected Results

After applying all optimizations:

| Asset | Before | After | Savings |
|-------|--------|-------|---------|
| bundle.js | 37KB | ~18KB | 52% |
| bundle.css | 2.2KB | ~0.9KB | 59% |
| polyfills.js | 6.9KB | ~3.5KB | 49% |
| **Total** | **47KB** | **~22KB** | **53%** |

With Brotli compression:
- Additional 15-20% savings
- **Final compressed size: ~15-18KB**

## 12. Troubleshooting

### Build Errors
```bash
# If Terser fails:
npm install terser-webpack-plugin@latest

# If Brotli fails:
node -e "console.log(require('zlib').brotliCompressSync)"
```

### Size Regression
```bash
# Compare builds:
ls -la build/*.{js,css} > before.txt
# ... make changes ...
ls -la build/*.{js,css} > after.txt
diff before.txt after.txt
```

### Performance Impact
- **Build Time**: +10-20% due to advanced optimization
- **Runtime Performance**: Improved due to smaller payload
- **Memory Usage**: Reduced due to dead code elimination

## 13. Monitoring

Set up automated size monitoring:
```bash
#!/bin/bash
# size-monitor.sh
CURRENT_SIZE=$(npm run size-check 2>/dev/null | grep "Total size" | awk '{print $3}')
MAX_SIZE=25000  # 25KB limit

if [ "$CURRENT_SIZE" -gt "$MAX_SIZE" ]; then
  echo "ERROR: Bundle size ${CURRENT_SIZE}B exceeds limit ${MAX_SIZE}B"
  exit 1
fi
```

This comprehensive optimization strategy should reduce your ESP32 web assets by 50-60%, making them much more suitable for embedded deployment while maintaining all functionality.
