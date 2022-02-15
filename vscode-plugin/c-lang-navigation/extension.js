
const vscode = require('vscode');
const { spawn } = require('child_process');
const readline = require('readline');


class CodeNavigation {
	constructor() {
		let serverPath = vscode.workspace.getConfiguration("c-lang-navigation").get("serverPath");
		this.handle = spawn(serverPath)
		this.state = 'ready'
		console.log("c-lang-navigation initialized.")
		this.io = readline.createInterface({
			input: this.handle.stdout,
		});

		this.io.on('line', this.onResponse);

	}

	sendIndex() {
		let codeRoot = vscode.workspace.getConfiguration("c-lang-navigation").get("rootIndexPath")
		let excludes = vscode.workspace.getConfiguration("c-lang-navigation").get("excludePatterns")

		this.handle.stdin.write(JSON.stringify({ "command": "index", "payload": { "path": codeRoot, "excludes": excludes } }) + "\n")
		this.state = 'indexing';

		vscode.window.showInformationMessage('Indexing...');
	}

	sendResolve() {
		let document = vscode.window.activeTextEditor.document;

		let range = document.getWordRangeAtPosition(vscode.window.activeTextEditor.selection.active);

		let line = range.start.line;
		let column = range.start.character;
		let path = document.fileName;

		this.handle.stdin.write(JSON.stringify({ "command": "resolve", "payload": { "path": path, "line": line, "column": column } }) + "\n");
	}

	sendFindUsages() {
		let document = vscode.window.activeTextEditor.document;

		let range = document.getWordRangeAtPosition(vscode.window.activeTextEditor.selection.active);

		let line = range.start.line;
		let column = range.start.character;
		let path = document.fileName;

		this.handle.stdin.write(JSON.stringify({ "command": "find_usages", "payload": { "path": path, "line": line, "column": column } }) + "\n");
	}

	async onResponse(line) {
		try {
			let data = JSON.parse(line);

			if (data.command == 'index') {
				vscode.window.showInformationMessage(`${data.status}, time: ${data.time_ms} ms`);
			}
			else if (data.command == 'resolve') {
				if (data.status == "not_found") {
					vscode.window.showInformationMessage(`Not found!`);
				}
				else {
					await vscode.commands.executeCommand("vscode.open", vscode.Uri.parse(data.coordinate.path));
					let editor = vscode.window.activeTextEditor;
					let range = editor.document.lineAt(data.coordinate.line).range;
					editor.selection = new vscode.Selection(range.start, range.end);
					editor.revealRange(range);

				}
			}
			else if (data.command == 'find_usages') {
				let panel = vscode.window.createWebviewPanel(
					'c-lang-references',
					'Definition References',
					vscode.ViewColumn.Two,
					{
						enableScripts: true
					}
				);

				panel.webview.onDidReceiveMessage(async message => {
					if (message.command === "open") {
						const uri = vscode.Uri.parse(message.link);
						const line = (+uri.fragment.substring(1)) - 1;
						const editor = await vscode.window.showTextDocument(uri);
						editor.revealRange(new vscode.Range(line, 0, line, 0), vscode.TextEditorRevealType.InCenterIfOutsideViewport);
					}
				});

				let inner = "";
				for(let c of data.coordinates){
					inner += `<p><a href="c-lang-ref://${c.path}#${c.line}">${c.path}:${c.line}</a></p>`;
				}

				panel.webview.html = `
				
<!DOCTYPE html>
<html lang="en">
<head>
    <title>Usages</title>
</head>
<body>
    ${inner}
</body>
<script>
const vscode = acquireVsCodeApi();

for (const link of document.querySelectorAll('a[href^="c-lang-ref:"]')) {
    link.addEventListener('click', () => {
        vscode.postMessage({
            command: "open",
            link: link.getAttribute('href').replace('c-lang-ref', 'file'),
        });
    });
}
</script>
</html>
				`;

			}
		}
		catch (e) {
			console.log(e);
			return;
		}
	}

	finalize() {
		this.handle.kill();
	}
}

let codeNavigation = new CodeNavigation();

function activate(context) {



	let indexCmd = vscode.commands.registerCommand('c-lang-navigation.index', function () {

		codeNavigation.sendIndex();

	});

	context.subscriptions.push(indexCmd);

	let resolveCmd = vscode.commands.registerCommand('c-lang-navigation.resolve', function () {

		codeNavigation.sendResolve();

	});

	context.subscriptions.push(resolveCmd);

	let findUsagesCmd = vscode.commands.registerCommand('c-lang-navigation.find_usages', function () {

		codeNavigation.sendFindUsages();

	});

	context.subscriptions.push(findUsagesCmd);
}

// this method is called when your extension is deactivated
function deactivate() {
	codeNavigation.finalize();
}

module.exports = {
	activate,
	deactivate
}
