
/*
	MIT License

	Copyright (c) 2021-2023 JulianDroid

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

/*
	This file is intented to be run under Nodejs
*/

const fs = require('fs');
const os = require('os');
const libpath = require('path');

/* utils */
const log = (...args)=>{
	args[0] = `=> ${args[0]}`;
	console.log(...args);
};
const warn = (...args)=>{
	args[0] = `WARN: ${args[0]}`;
	console.log(...args);
};
const hashstr = str=>{
	let hash = 0;
	if (str.length == 0) return hash;
	for (let i = 0; i < str.length; ++i) {
		let chr = str.charCodeAt(i);
		hash = ((hash << 5) - hash) + chr;
		hash |= 0;
	}
	return hash < 0? -hash: hash;
}

/* arg settings */
let optSourceFiles = [];
let optCode = 'c';
let optLang = 'en_US';
let optOutputDir = './multilang';
let optRecover = false;

const ARG_TABLE = {
	h: ()=>{
		console.log((`
			multilang-ify tool for source code
			Copyright (c) 2021-2023 Julian Droske

			Usage:
				${process.argv[0]} [options] <source_file> ...
			Options:
				-h:     show help
				-c:     set source code language = c
				        supports: c
				-r:     recover backuped source code
				-o:     output directory, defaults to ./multilang
				-l:     set target language = en_US
				        output file will be under output_directory/target_language/
		`).replaceAll('\t\t\t', ''));
		os.exit(1);
	},
	c: nextopt=>optCode = nextopt(),
	r: ()=>optRecover = true,
	l: nextopt=>optLang = nextopt(),
	o: nextopt=>optOutputDir = nextopt(),
};

const PROCESSORS = {
	c: {
		main(fileName, fileData, outputDir){
			log('simplifying code');
			let procData = fileData.
				replace(new RegExp('/\\*[\\s\\S]*?\\*/', 'g'), '');
				// replace(new RegExp('//.*', 'g'), '');

			log('collecting data');
			let texts = procData.match(new RegExp('.*"', 'g'));
			log(`found ${texts.length} raw strings`);

			let symbols = {};

			log('compiling strings');
			texts.forEach(line=>{
				/* header files */
				if(line.match(new RegExp('.*#.*include.*"'))) return;

// console.log('STRAT')
// console.log(line.match(new RegExp('"(\\\\.|[^"\n])*"', 'g')))
// console.log(line)
// console.log('END')
				let matches = line.match(new RegExp('"(\\\\.|[^"\n])*"', 'g'));
				if(matches) matches.forEach(text=>{
					if(text.length==2) return;

					let [fullText, fullString, string] = text.match(new RegExp('("((\\\\.|[^"\n])*)")'));


					let preSymbol = string.replace(/[^a-zA-Z0-9]/g, c=>`${c.charCodeAt()}`);
					let symbolStart = preSymbol.substr(0, 5);
					let symbolEnd = preSymbol.substr(preSymbol.length > 5? preSymbol.length - 5: 0);
					let symbolHash = hashstr(preSymbol);
					let minSymbol = `str_${symbolStart}_${string.length}_${symbolEnd}_${symbolHash}`;

					// if(symbols[minSymbol]) return;
										// console.log('>>>', fullString, '<<<');

					fileData = fileData.replace(fullString, minSymbol);

					symbols[minSymbol] = string;
				})
			});
			log(`processed ${Object.keys(symbols).length} strings`);
			// let newData = fileData.replace(new RegExp('\n(.*?)"([\\s\\S]*?[^\\\\])"', 'g'), (fullString, before, string)=>{
				// if(before.match(/[\s]*#.*include/)) console.log('HI\nHI\nHI')
			// });
			// console.log(newData)

			let outputName = `${fileName}.h`;

			log('generating header...');
			let headerData = Object.entries(symbols).map(([symbol, string])=>`#define ${symbol} "${string}"`).join('\n');

			/* add header */
			fileData = `#include "${outputDir}/${outputName}"\n` + fileData;

			return {
				outputName,
				outputData: headerData,
				sourceData: fileData
			};
		}
	}
};

/* parse args */

{
	let argv = process.argv.slice(2);
	let i = 0;
	let nextopt = ()=>argv[++i];
	for(; i < argv.length; ++i){
		let arg = argv[i];

		if(!arg.startsWith('-')){
			optSourceFiles = argv.slice(i);
			break;
		}

		let optfunc = ARG_TABLE[arg[1]];
		if(!optfunc){
			console.log(`invalid argument: '${arg}'`);
			ARG_TABLE.h();
		}
		optfunc(nextopt);
	}
}

for(let sourceFile of optSourceFiles){
	let fileData = fs.readFileSync(sourceFile).toString();
	let name = libpath.basename(sourceFile).replace(/\..+?$/, '');
	let outputDir = `${optOutputDir}/${optLang}`;

	let backupFile = `${sourceFile}.non-multilang`;

	if(optRecover){
		if(!fs.existsSync(backupFile)){
			warn(`skipped recovering ${sourceFile}: file not found`);
			continue;
		}
		fs.copyFileSync(backupFile, sourceFile);
		fs.rmSync(backupFile);
	}else{
		let {outputName, outputData, sourceData} = PROCESSORS[optCode].main(name, fileData, outputDir);

		/* backup first */
		if(fs.existsSync(backupFile)){
			warn(`skipped ${sourceFile}: file exists`);
			continue;
		}
		fs.copyFileSync(sourceFile, backupFile);

		if(!fs.existsSync(outputDir)) fs.mkdirSync(outputDir, {
			recursive: true
		});
		fs.writeFileSync(sourceFile, sourceData);
		fs.writeFileSync(`${outputDir}/${outputName}`, outputData);
	}
	log(`${sourceFile} processed`);
}
