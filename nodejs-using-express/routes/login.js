var express = require('express');
var router = express.Router();

/* GET protected page page. */
router.get('/', function(req, res, next) {
    res.redirect('/protected/protected.html');  
});

module.exports = router;
