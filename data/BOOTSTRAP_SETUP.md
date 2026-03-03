# Bootstrap Setup Instructions

The web interface requires Bootstrap 5.3 CSS and JS files to be placed in the `data/` directory.

## Required Files

You need to download and place the following files in this directory:

1. **bootstrap.min.css** - Bootstrap 5.3 CSS (minified)
2. **bootstrap.bundle.min.js** - Bootstrap 5.3 JS with Popper (minified)

## Download Instructions

### Option 1: Direct Download from CDN

Visit the Bootstrap CDN and download the files:

1. Go to https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css
   - Save as `bootstrap.min.css` in the `data/` directory

2. Go to https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js
   - Save as `bootstrap.bundle.min.js` in the `data/` directory

### Option 2: Download from Bootstrap Website

1. Visit https://getbootstrap.com/docs/5.3/getting-started/download/
2. Download the "Compiled CSS and JS" package
3. Extract the archive
4. Copy the following files to the `data/` directory:
   - `css/bootstrap.min.css` → `data/bootstrap.min.css`
   - `js/bootstrap.bundle.min.js` → `data/bootstrap.bundle.min.js`

### Option 3: Using npm (if you have Node.js installed)

```bash
npm install bootstrap@5.3.0
cp node_modules/bootstrap/dist/css/bootstrap.min.css data/
cp node_modules/bootstrap/dist/js/bootstrap.bundle.min.js data/
```

## File Sizes (Approximate)

- `bootstrap.min.css`: ~190 KB (minified)
- `bootstrap.bundle.min.js`: ~80 KB (minified)

## Gzip Compression

For optimal performance on ESP32, you can gzip these files:

```bash
gzip -k bootstrap.min.css
gzip -k bootstrap.bundle.min.js
```

This will create:
- `bootstrap.min.css.gz` (~25 KB)
- `bootstrap.bundle.min.js.gz` (~15 KB)

The web server can serve these compressed versions automatically if configured.

## Verification

After placing the files, your `data/` directory should contain:

```
data/
├── BOOTSTRAP_SETUP.md (this file)
├── README.md
├── bootstrap.min.css
├── bootstrap.bundle.min.js
├── style.css
├── index.html
├── config-gpio.html
├── config-timing.html
├── config-system.html
├── app.js
├── config-gpio.js
├── config-timing.js
└── config-system.js
```

## Uploading to ESP32

Once all files are in place, upload them to the ESP32 SPIFFS/LittleFS:

Using PlatformIO:
```bash
pio run --target uploadfs
```

Or using Arduino IDE:
- Tools → ESP32 Sketch Data Upload

## Alternative: Embedded in Code

If you prefer to embed Bootstrap directly in the firmware (not recommended due to size), you can:

1. Convert the files to C header files using `xxd` or similar tools
2. Include them in the firmware and serve from PROGMEM
3. This will increase firmware size significantly (~200KB+)

## Dark Theme

The interface uses Bootstrap's built-in dark theme via the `data-bs-theme="dark"` attribute on the `<html>` tag. Additional custom dark theme styles are in `style.css`.
