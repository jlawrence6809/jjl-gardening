const webpack = require('webpack');
const path = require('path');
import ESPBuildPlugin from './esp/esp-build-plugin.js';

export default {
  webpack(config, env, helpers, options) {
    if (env.isProd) {
      config.devtool = false;
      config.output = {
        ...config.output,
        path: path.resolve(__dirname, 'build'),
      };
      config.plugins = [
        ...config.plugins,
        new ESPBuildPlugin({
          exclude: [
            '200.html',
            'preact_prerender_data.json',
            'push-manifest.json',
          ],
        }),
      ];
    }
  },
};
