# Contributing to SmallTV RSS

First off, thank you for considering contributing to SmallTV RSS! 🎉

This document provides guidelines for contributing to this project. Following these guidelines helps communicate that you respect the time of the developers managing and developing this open source project.

---

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How Can I Contribute?](#how-can-i-contribute)
- [Style Guidelines](#style-guidelines)
- [Commit Message Guidelines](#commit-message-guidelines)
- [Pull Request Process](#pull-request-process)

---

## 🤝 Code of Conduct

This project adheres to a Code of Conduct that all contributors are expected to follow:

- **Be respectful** and inclusive in your language
- **Be patient** with newcomers and questions
- **Be constructive** in your criticism
- **Focus on** what is best for the community
- **Show empathy** towards other community members

---

## 🚀 Getting Started

### Prerequisites
- Arduino IDE (1.8.x or later) or PlatformIO
- ESP8266 board support installed
- Basic knowledge of C++ and Arduino programming
- Git for version control

### Setting Up Your Development Environment
1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/SmallTV_RSS.git
   cd SmallTV_RSS
   ```
3. Add the upstream repository:
   ```bash
   git remote add upstream https://github.com/ORIGINAL_OWNER/SmallTV_RSS.git
   ```
4. Create your `wifi_secrets.h` and `secrets.h` from example files
5. Test that the project compiles successfully

---

## 💡 How Can I Contribute?

### Reporting Bugs 🐛

Before creating a bug report, please check existing issues to avoid duplicates.

When filing a bug report, include:
- **Clear title** describing the issue
- **Steps to reproduce** the problem
- **Expected behavior** vs actual behavior
- **Hardware info**: ESP8266 variant, display model
- **Software versions**: Arduino IDE, library versions
- **Serial output** if relevant (use code blocks)
- **Screenshots** if applicable

### Suggesting Enhancements ✨

Enhancement suggestions are welcome! Include:
- **Clear use case** for the feature
- **Why it would be useful** to most users
- **Possible implementation** approach (if you have ideas)
- **Alternative solutions** you've considered

### Pull Requests 🔧

We actively welcome your pull requests!

**Good first issues** for newcomers:
- Documentation improvements
- Adding code comments
- Fixing typos
- Adding configuration options
- Improving error messages

**More advanced contributions**:
- New display modes/scenes
- Additional weather data sources
- Support for other news feeds
- Performance optimizations
- New web UI features

---

## 📝 Style Guidelines

### C++ Code Style

Follow these conventions for consistency:

#### Naming Conventions
```cpp
// Constants: UPPER_CASE with underscores
constexpr int MAX_RETRY_COUNT = 3;

// Variables: camelCase
int retryCount = 0;
unsigned long lastUpdate = 0;

// Functions: camelCase (verb-first)
void fetchWeatherData();
bool validateInput(String input);

// Classes/Structs: PascalCase
struct WeatherData {
  float temperature;
  int humidity;
};

// Enum classes: PascalCase with UPPER_CASE values
enum class Status : uint8_t {
  IDLE,
  CONNECTING,
  CONNECTED
};
```

#### Formatting
```cpp
// Braces: Opening brace on same line (K&R style)
void myFunction() {
  if (condition) {
    doSomething();
  } else {
    doSomethingElse();
  }
}

// Indentation: 2 spaces (no tabs)
void exampleFunction() {
  if (condition) {
    for (int i = 0; i < 10; i++) {
      processItem(i);
    }
  }
}

// Line length: Aim for 80-100 characters maximum
// Break long lines at logical points
void longFunctionCall(int param1, 
                      int param2, 
                      int param3);

// Space after keywords, around operators
if (x > 0) {
  result = a + b * c;
}
```

#### Comments
```cpp
// Use single-line comments for brief explanations
int count = 0;  // Counter for retry attempts

// Use multi-line comments for function/block documentation
// functionName()
// Brief description of what the function does
// Parameters:
//   param1 - Description of first parameter
//   param2 - Description of second parameter
// Returns: Description of return value
int functionName(int param1, int param2) {
  // Implementation
}

// Add inline comments for complex logic
if ((status == CONNECTED) && (retries < MAX_RETRIES)) {
  // Only retry if we're connected and haven't exceeded limit
  performRetry();
}
```

### HTML/CSS/JavaScript Style

For web interface code:
- **Indentation**: 2 or 4 spaces consistently
- **Naming**: camelCase for JavaScript, kebab-case for CSS
- **Comments**: Explain non-obvious logic
- **Modern syntax**: Use ES6+ features (arrow functions, const/let, template literals)

---

## 📬 Commit Message Guidelines

### Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types
- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation changes
- **style**: Formatting, missing semicolons, etc. (no code change)
- **refactor**: Code restructuring (no functional change)
- **perf**: Performance improvements
- **test**: Adding or updating tests
- **chore**: Maintenance tasks, dependency updates

### Examples
```
feat(weather): add support for hourly forecast

- Fetch hourly data from OpenWeatherMap API
- Display next 6 hours on new screen
- Add configuration for update interval

Closes #42
```

```
fix(wifi): handle connection timeout correctly

Previously the device would hang indefinitely if WiFi
was unavailable. Now it properly times out after 12 seconds
and continues in degraded mode.

Fixes #38
```

```
docs(readme): add troubleshooting section

Added common issues and solutions for:
- WiFi connection failures
- NTP sync problems
- Weather API errors
```

---

## 🔄 Pull Request Process

### Before Submitting
1. **Test thoroughly** on real hardware
2. **Update documentation** if you changed functionality
3. **Follow style guidelines** outlined above
4. **Ensure code compiles** without warnings
5. **Check memory usage** doesn't exceed limits
6. **Add comments** for complex code
7. **Update README** if adding new features

### Submitting
1. **Fork** the repository
2. **Create a branch** from `main`:
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. **Make your changes** with clear commit messages
4. **Push to your fork**:
   ```bash
   git push origin feature/my-new-feature
   ```
5. **Open a Pull Request** with:
   - Clear title describing the change
   - Detailed description of what and why
   - Reference to related issues (if any)
   - Screenshots/videos if UI changes
   - Test results on hardware

### Pull Request Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Tested on real hardware
- [ ] Tested with WiFi connection/disconnection
- [ ] Tested with API failures
- [ ] Checked memory usage
- [ ] Verified no compile warnings

## Screenshots (if applicable)
[Add screenshots here]

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-reviewed my own code
- [ ] Commented complex/hard-to-understand areas
- [ ] Updated documentation
- [ ] Changes generate no new warnings
- [ ] Added tests (if applicable)
```

### Review Process
- Maintainers will review your PR
- Address any feedback or requested changes
- Once approved, a maintainer will merge your PR
- Your contribution will be credited in the changelog

---

## ❓ Questions?

If you have questions about contributing:
- Check existing [GitHub Issues](https://github.com/YOUR_USERNAME/SmallTV_RSS/issues)
- Start a [GitHub Discussion](https://github.com/YOUR_USERNAME/SmallTV_RSS/discussions)
- Contact the maintainers

---

## 🙏 Thank You!

Your contributions make this project better for everyone. We appreciate your time and effort! ⭐

---

<p align="center">
  <sub>Happy coding! 🚀</sub>
</p>
