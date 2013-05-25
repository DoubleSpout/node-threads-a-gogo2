module.exports = function(string){
	if(string === 'false') return false;
	else if(string === 'true') return true;
	else if(string === 'undefined') return;
			
	if(isFinite(string)) return string-0;

	try{
		var res = JSON.parse(string)
		
	}
	catch(e){
		var res =  string;
	}

	return res;
}