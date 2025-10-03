import express from 'express';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const port = process.env.PORT || 3000;
const staticRoot = path.join(__dirname, 'www');

app.use(express.static(staticRoot));

app.get('*', (_req, res) => {
    res.sendFile(path.join(staticRoot, 'index.html'));
});

app.listen(port, '0.0.0.0', () => {
    console.log(`Nasreddins Camera Server läuft auf http://0.0.0.0:${port}`);
});
