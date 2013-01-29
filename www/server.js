/**
   Main application.
**/

var express = require('express');
var app = express();
var hbs = require('express-hbs');

app.set('views', './views');

// Setup Express to use the Handlerbars engine for views by default
// and those ending in .hbs.
app.engine('hbs', hbs.express3({
    partialsDir: __dirname + '/views/partials',
    layoutsDir: __dirname + '/views/layout'
}));
app.set('view engine', 'hbs');
app.set('views', __dirname + '/views');

app.use("/public", express.static(__dirname + '/public'));

app.get('/', function(req, res){
    res.render('index', {
	title: 'Introducing sGrok'
    });
});

app.listen(3000);
console.log('Listening on port 3000');
