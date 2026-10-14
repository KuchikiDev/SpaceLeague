@echo off
rem Ouvre le site de reglage des variables de gameplay ORA.
start "" http://localhost:5178
node "%~dp0server.js"
