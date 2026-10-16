/**
 * ESLint configuration for the Transalp Companion App.
 *
 * Lints JavaScript files only. TypeScript files are covered by tsc --noEmit
 * in the CI pipeline. Adding @typescript-eslint/parser would require extra
 * dependencies — tsc already enforces type safety.
 */

module.exports = [
  {
    ignores: [
      'node_modules/',
      'dist/',
      'web-build/',
      '.expo/',
      '**/*.ts',
      '**/*.tsx',
    ],
  },
  {
    rules: {
      'no-console': ['warn', { allow: ['warn', 'error'] }],
      'no-debugger': 'error',
      'no-unused-vars': 'off',
      'prefer-const': 'error',
      'no-var': 'error',
      'eqeqeq': ['error', 'always'],
    },
  },
];
