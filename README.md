# Venky Agent v1

Personal AI agent by Venky.

## Free-first architecture

User → Agent Brain → Tasks → Tools → Research → Action → Daily Report

### AI provider options

- `demo`: no API, works immediately
- `ollama`: local AI on your laptop; no cloud API bill
- `gemini`: optional cloud provider using a user-supplied key

### Security

Never commit a real API key into GitHub. Keep secrets in a local `config.js` that is ignored by Git.

## Next

1. Run the web UI.
2. Choose a free AI provider.
3. Add the GitHub tools layer.
4. Add persistence and real agent actions.
