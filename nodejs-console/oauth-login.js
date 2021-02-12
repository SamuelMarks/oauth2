// This is a console style Nodejs version of oauth to illustrate we do not 
// need frameworks like vue or axios.

const { exec } = require("child_process");
const https = require("https");
const querystring = require("querystring")

// TODO change this to a random string
const common_secret = Math.random().toString(36).substring(7);
console.log(`Random Secret: ${common_secret}`)
let appAuth = null;
let accessToken = null;
const redirectUri = "http://localhost:3000/ibm/cloud/appid/callback"

function open_browser_for_authorisation_url(appAuth)
{
    console.log(appAuth)
    const endpoint = appAuth.endpoint;
    const clientId = appAuth.clientId;
    const query_params = {
        response_type: 'code',
        client_id: clientId,
        redirect_uri: redirectUri,
        state: common_secret,
        scope: 'openid'
    }

    const authorize_url = endpoint+'/authorization?'+querystring.stringify(query_params)
    console.log(`open browser to ${authorize_url}`)
    // open browser
    exec("xdg-open '"+authorize_url+"'")
}

https.get('https://31f5ff35.eu-gb.apigw.appdomain.cloud/authtest/GetApplicationEndpoint',
            (res) => {
                console.log(res.statusCode);
                let bytes = ""
                res.on('data', (d) => {
                    bytes += d
                })
                res.on('end', () => {
                    appAuth = JSON.parse(bytes)
                    open_browser_for_authorisation_url(appAuth)
                })
            }
).on('error', (err) => {
    console.log(error)
})

function get_access_token(code) 
{
    const data = JSON.stringify({
        grant_type: 'authorization_code',
        code: code,
        redirect_uri: redirectUri,
        client_id: appAuth.clientId
    })
    const options = {
        host: "31f5ff35.eu-gb.apigw.appdomain.cloud",
        port: 443,
        path: "/authtest/GetAccessToken",
        method: "post",
        headers: {
            'Content-type': 'application/json',
            'Content-length': data.length
        }
    }

    const req = https.request(options,
            (res) => {
                console.log(res.statusCode);
                let bytes = ""
                res.on('data', (d) => {
                    bytes += d
                })
                res.on('end', () => {
                    let body = JSON.parse(bytes)
                    console.log(body)
                    access_token = body

                    // from here the rest is similar to the php
                })
            }
    ).on('error', (err) => {
        console.log(error)
    })
    req.write(data)
    req.end()
}

// note that we receive in HTTP not HTTP
const http = require('http')
const url = require('url')
http.createServer((req,res) => {
    let bytes = ""
    const queryObject = url.parse(req.url, true).query
    // console.log(queryObject)
    req.on('data', () => {

    });
    req.on('end', () => {
        res.writeHead(200, {'Content-Type': 'text/html'})
        // console.log(queryObject.state)
        // console.log(common_secret)
        if ( queryObject.state == common_secret) {
            res.end('GET received, secret confirmed')
        } else {
            res.end("GET failed, different secret")
        }
        console.log("Code: "+queryObject.code)

        get_access_token(queryObject.code)
    })
}).listen(3000)

