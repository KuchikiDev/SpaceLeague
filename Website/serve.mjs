import {createServer} from 'node:http';
import {readFile, stat} from 'node:fs/promises';
import {resolve, extname, sep} from 'node:path';
const root = resolve(import.meta.dirname, 'dist');
const types = {'.html':'text/html; charset=utf-8','.css':'text/css; charset=utf-8','.js':'text/javascript; charset=utf-8','.webp':'image/webp','.woff2':'font/woff2','.svg':'image/svg+xml','.txt':'text/plain; charset=utf-8'};
createServer(async (request, response) => {
  try {const path = decodeURIComponent(new URL(request.url, 'http://localhost').pathname);let file = resolve(root, '.' + (path === '/' ? '/index.html' : path));if (!file.startsWith(root + sep)) throw new Error('Outside public root');if ((await stat(file)).isDirectory()) file=resolve(file,'index.html');const body = await readFile(file);response.writeHead(200, {'Content-Type':types[extname(file)] || 'application/octet-stream','Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});response.end(body);} catch {response.writeHead(404, {'Content-Type':'text/plain; charset=utf-8'});response.end('Introuvable');}
}).listen(4173, '127.0.0.1', () => console.log('ORA preview: http://127.0.0.1:4173'));
