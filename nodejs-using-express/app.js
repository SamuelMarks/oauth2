var createError = require('http-errors');
var express = require('express');
var path = require('path');
var cookieParser = require('cookie-parser');
var logger = require('morgan');

const passport = require("passport");         // npm add passport
const appID = require("ibmcloud-appid");      // npm add ibmcloud-appid
const session = require("express-session");   // npm add express-session (https://www.npmjs.com/package/express-session)

const WebAppStrategy = appID.WebAppStrategy;
const AppIdCallbackUrl = "/ibm/cloud/appid/callback";
const port = process.env.PORT || 3000;
const session_secret = "123456"; // TODO make a random number

const indexRouter = require('./routes/index');
const usersRouter = require('./routes/users');
const loginRouter = require('./routes/login');
// let { config } = require('process');
let config = null

const app = express();
app.set('port', 8080)

// -------------------------------------------------
// Standard setup in a default express application
// -------------------------------------------------

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'jade');

app.use(logger('dev'));
app.use(express.json());
app.use(express.urlencoded({ extended: false }));
app.use(cookieParser());
app.use(express.static(path.join(__dirname, 'public')));

app.use('/', indexRouter);
app.use('/users', usersRouter);





// error handler
app.use(function(err, req, res, next) {
  // set locals, only providing error in development
  res.locals.message = err.message;
  res.locals.error = req.app.get('env') === 'development' ? err : {};

  // render the error page
  res.status(err.status || 500);
  res.render('error');
});

// -------------------------------------------------
// IBM Cloud App ID code modified from the demo app
// -------------------------------------------------

// Setup express application to use express-session middleware
// Must be configured with proper session storage for production
// environments. See https://github.com/expressjs/session for
// additional documentation
app.use(session({
	secret: session_secret,   // this is the same as our secret from other scripts
	resave: true,
	saveUninitialized: true,
	proxy: true
}));

// Configure express application to use passportjs
app.use(passport.initialize());
app.use(passport.session());

let webAppStrategy = new WebAppStrategy(getAppIDRedirectUri());
passport.use(webAppStrategy);


// Configure passportjs with user serialization/deserialization. This is required
// for authenticated session persistence accross HTTP requests. See passportjs docs
// for additional information http://passportjs.org/docs
passport.serializeUser((user, cb) => cb(null, user));
passport.deserializeUser((obj, cb) => cb(null, obj));

// Callback to finish the authorization process. Will retrieve access and identity tokens/
// from AppID service and redirect to either (in below order)
// 1. the original URL of the request that triggered authentication, as persisted in HTTP session under WebAppStrategy.ORIGINAL_URL key.
// 2. successRedirect as specified in passport.authenticate(name, {successRedirect: "...."}) invocation
// 3. application root ("/")
app.get(AppIdCallbackUrl, passport.authenticate(WebAppStrategy.STRATEGY_NAME, {failureRedirect: '/error'}));

// Protect everything under /protected
app.use("/login", passport.authenticate(WebAppStrategy.STRATEGY_NAME));
app.use('/login', loginRouter);

app.use('/login', loginRouter);


app.use("/protected", passport.authenticate(WebAppStrategy.STRATEGY_NAME));

// This will statically serve the protected page (after authentication, since /protected is a protected area):
app.use('/protected', express.static("protected"));

app.get("/logout", (req, res) => {
	WebAppStrategy.logout(req);
	res.redirect("/");
});

// This route must be last otherwise you will get spurious 404's
// catch 404 and forward to error handler
app.use(function(req, res, next) {
  next(createError(404));
});

function getAppIDRedirectUri() {
	try {
		// if running locally we'll have the local config file
    config = require('./localdev-config.json');
    // this should modified to have the domain name of your server instance
    // if this is actually running as a node service
    config.redirectUri = 'http://localhost:3000/ibm/cloud/appid/callback';
	} catch (e) {
    console.log(e)
		if (process.env.APPID_SERVICE_BINDING) { // if running on Kubernetes this env variable would be defined
			config = JSON.parse(process.env.APPID_SERVICE_BINDING);
			config.redirectUri = process.env.redirectUri;
		} else { // running on CF
			let vcapApplication = JSON.parse(process.env["VCAP_APPLICATION"]);
			return {"redirectUri" : "https://" + vcapApplication["application_uris"][0] + AppIdCallbackUrl};
		}
	}
	return config;
}

module.exports = app;
