const webpack = require('webpack');
const path = require('path');
const TerserPlugin = require('terser-webpack-plugin');
const ESPBuildPlugin = require('./esp/esp-build-plugin.js').default;

module.exports = {
  webpack(config, env, helpers, options) {
    if (env.isProd) {
      // Disable source maps entirely for ESP32
      config.devtool = false;
      
      config.output = {
        ...config.output,
        path: path.resolve(__dirname, 'build'),
      };

      // Enhanced optimizations for ESP32 (work with existing Preact CLI config)
      if (config.optimization && config.optimization.minimizer) {
        // Enhance existing Terser configuration
        config.optimization.minimizer.forEach(minimizer => {
          if (minimizer.constructor.name === 'TerserPlugin') {
            // Enhance existing terser options
            minimizer.options = minimizer.options || {};
            minimizer.options.terserOptions = {
              ...minimizer.options.terserOptions,
              compress: {
                ...minimizer.options.terserOptions?.compress,
                drop_console: true,        // Remove console.log
                drop_debugger: true,      // Remove debugger  
                unused: true,             // Remove unused vars
                dead_code: true,          // Remove unreachable code
                passes: 2,                // Multiple optimization passes
              },
              mangle: {
                ...minimizer.options.terserOptions?.mangle,
                toplevel: true,           // Mangle top-level names
              },
            };
          }
        });
      }

      // Enhanced tree shaking
      config.optimization = {
        ...config.optimization,
        usedExports: true,
        sideEffects: false,
      };

      // Module resolution optimizations (removed aggressive aliasing that breaks imports)
      // Keep existing resolve configuration

      // Remove unnecessary plugins for ESP32 (but keep HtmlWebpackPlugin as others depend on it)
      config.plugins = config.plugins.filter(plugin => {
        // Remove plugins that generate files we don't need
        const pluginName = plugin.constructor.name;
        return ![
          'GenerateSW', 'InjectManifest',     // Service worker plugins
          // Keep HtmlWebpackPlugin as other plugins depend on it
        ].includes(pluginName);
      });

      // Configure ESP build plugin with optional Brotli compression
      const espPluginOptions = {
        exclude: [
          '200.html',
          'preact_prerender_data.json',
          'push-manifest.json',
          /\.map$/,                   // Source maps (regex pattern)
          'manifest.json',            // PWA manifest
          'sw.js',                    // Service worker
        ],
        useBrotli: process.env.BROTLI === 'true',  // Enable Brotli if env var set
        compressionLevel: process.env.BROTLI === 'true' ? 11 : 9, // Max compression
      };

      config.plugins = [
        ...config.plugins,
        new ESPBuildPlugin(espPluginOptions),
      ];

      // Add bundle analyzer if requested
      if (process.env.ANALYZE === 'true') {
        const BundleAnalyzerPlugin = require('webpack-bundle-analyzer').BundleAnalyzerPlugin;
        config.plugins.push(new BundleAnalyzerPlugin({
          analyzerMode: 'static',
          openAnalyzer: false,
          reportFilename: 'bundle-report.html',
        }));
      }
    }
  },
};
