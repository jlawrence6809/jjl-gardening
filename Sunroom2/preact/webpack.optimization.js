// Advanced webpack optimizations for ESP32 builds
const TerserPlugin = require('terser-webpack-plugin');

export const esp32Optimizations = {
  optimization: {
    minimize: true,
    minimizer: [
      new TerserPlugin({
        terserOptions: {
          compress: {
            // Remove all console.log statements
            drop_console: true,
            // Remove debugger statements
            drop_debugger: true,
            // Remove unreachable code
            dead_code: true,
            // Inline functions when beneficial
            inline: 2,
            // Remove unused variables
            unused: true,
            // Collapse single-use variables
            collapse_vars: true,
            // Reduce variable names to single characters
            reduce_vars: true,
            // More aggressive optimizations
            passes: 3,
            // Remove function names (saves space)
            keep_fnames: false,
            // Pure function annotations
            pure_funcs: ['console.log', 'console.info', 'console.warn'],
          },
          mangle: {
            // Mangle all property names (be careful with this)
            properties: {
              // Only mangle properties matching this regex
              regex: /^_/,
            },
            // Shorter variable names
            toplevel: true,
          },
          format: {
            // Remove all comments
            comments: false,
            // Minimize whitespace
            ascii_only: true,
          },
        },
        extractComments: false,
      }),
    ],
    // Tree shaking optimizations
    usedExports: true,
    sideEffects: false,
    // Split chunks for better caching (though not critical for ESP32)
    splitChunks: {
      chunks: 'all',
      minSize: 0,
      cacheGroups: {
        // Separate vendor code
        vendor: {
          test: /[\\/]node_modules[\\/]/,
          name: 'vendors',
          chunks: 'all',
          priority: 10,
        },
        // Separate common code
        common: {
          name: 'common',
          minChunks: 2,
          chunks: 'all',
          priority: 5,
        },
      },
    },
  },
  resolve: {
    // Use production versions of libraries
    alias: {
      'preact/compat': 'preact/compat/dist/compat.min.js',
      'preact': 'preact/dist/preact.min.js',
    },
  },
};
