# KaniMart

KaniMart is a native C++20 multi-seller marketplace backend built with Drogon, PostgreSQL, and a static frontend.

## Production setup

1. Install PostgreSQL, CMake 3.25+, Visual Studio 2022 (MSVC), and vcpkg.
2. Create the database and apply the initial schema:

	```powershell
	createdb -U postgres kanimart
	psql -U postgres -d kanimart -f db\migrations\001_initial.sql
	```

	Add a separately managed seed process for an initial administrator if your deployment needs one.
3. Copy `config.example.json` to a private `config.json` and replace the database connection values.
4. Set the required environment variables before starting the backend:

	```powershell
	$env:KANIMART_CONFIG_FILE = "C:\kanimart\config.json"
	$env:KANIMART_JWT_SECRET = "generate-a-long-random-secret"
	$env:KANIMART_ALLOWED_ORIGINS = "https://shop.example.com"
	$env:OPENAI_API_KEY = "your-openai-key"
	```

5. Configure the reverse proxy to serve `frontend/` and forward `/api/` to the backend on port `8080`. The frontend uses same-origin API requests in production. Set `window.KANIMART_API_BASE` before the page scripts only when the API is hosted on a separate origin.
6. Build and run the backend from a directory where the configured file is available:

	```powershell
	cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake
	cmake --build build --config Release --target kanimart
	.\build\Release\kanimart.exe
	```

Use HTTPS at the reverse proxy, keep `config.json` and all secrets outside source control, and run the backend under a process manager. The default local CORS behavior remains available when `KANIMART_ALLOWED_ORIGINS` is unset.

## Local development

Keep the existing `config.json`, set `KANIMART_JWT_SECRET`, start the backend on `127.0.0.1:8080`, and serve `frontend/` on port `5500`.

## Deploy on Render

1. Push this repository to GitHub or GitLab.
2. In Render, choose **New > Blueprint**, connect the repository, and apply `render.yaml`.
3. Render creates the web service and PostgreSQL database. Set `OPENAI_API_KEY` in the web service environment if KaMa AI is enabled.
4. Deploy and open the generated `onrender.com` URL. The web service serves the frontend and `/api/*` from the same origin.

The Docker startup script reads Render's `DATABASE_URL`, applies `db/migrations/001_initial.sql`, generates Drogon configuration using Render's `PORT`, and starts the API. The migration is idempotent, but production backups and a versioned migration runner are still recommended before changing the schema.
