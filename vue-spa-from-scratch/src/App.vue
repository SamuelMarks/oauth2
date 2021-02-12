<template>
  <div id="app">
    <h1>OAuth2 demo using Vue</h1>
    <button v-on:click="loginWithOAuth2">Who goes there?</button>
    <p>{{ private_message }}</p>
  </div>
</template>

<script>
// Reference: https://www.valentinog.com/blog/oauth2/
import randomstring from "randomstring"
import { encode as base64encode } from "base64-arraybuffer";
import { store } from "./store"
import AppID from "ibmcloud-appid-js"
import { IBM_APP_ID_CLIENTID, IBM_APP_ID_DISCOVERY_ENDPOINT } from "./env.js"

// here we build the authentication string that will send us off to IBM App ID
const clientId = IBM_APP_ID_CLIENTID
const discoveryEndpoint = IBM_APP_ID_DISCOVERY_ENDPOINT
const jsrsasign = require('jsrsasign');

// here we specify the return uri HOWEVER we could get this from the 
// browsers window.location.href variable, which is what the 
// ibmcloud-appid-js package does.
const redirectUri='http://localhost:8080/auth'
let authentication_url = null;

function save_state_and_verifier(state, codeVerifier) {
  // save the state and verifier ready for reload after sign in
  if ( window.location.search.includes("state") ) return;
  const storage = window.sessionStorage;
  storage.clear();
  storage.setItem("state", state);
  storage.setItem("codeVerifier", codeVerifier);
}

// required to bring in async/await babel functionality
import 'regenerator-runtime/runtime'
async function generatePKCECodeChallenge(codeVerifier) {
  const codeChallenge = jsrsasign.stob64(jsrsasign.KJUR.crypto.Util.sha256(codeVerifier))
  const nonce = getRandomString(20)
  return {
    codeVerifier: codeVerifier,
    codeChallenge: codeChallenge,
    nonce: nonce
  }
}

// Proto-code. Don't use for production
function buildLoginUrl(state, challenge) {
  const endpoint = "https://eu-gb.appid.cloud.ibm.com/oauth/v4/3143b98e-2035-4fac-b765-879402a1b585/authorization"
  const nonce = challenge.nonce

  const queryParams = {
    client_id: clientId, 
    response_type: "code",
    state: state,
    code_challenge: challenge.codeChallenge, 
    code_challenge_method: "S256",
    redirect_uri: redirectUri,
    nonce,
    scope: 'openid'
  };

  const getQueryParams = Object.keys(queryParams).map(function (key) {
			return encodeURIComponent(key) + '=' + encodeURIComponent(queryParams[key]);
		}).join('&');

  const url = endpoint + '?' + getQueryParams
  return url
}

async function getOAuthAccessToken() {
  const currentLocation = new URL(window.location.href);
  const authorizationCode = currentLocation.searchParams.get("code");
  const stateFromLocation = currentLocation.searchParams.get("state");
  const initialCodeVerifier = window.sessionStorage.getItem("codeVerifier");
  

  // This is a good place for checking the state too  
  console.log(window.sessionStorage.getItem("state"))
  console.log(stateFromLocation)
  if (window.sessionStorage.getItem("state") !== stateFromLocation){
    throw Error("Probable session hijacking attack!");
  }

  console.log("Code: "+authorizationCode)
  const queryParams = {
    grant_type: 'authorization_code',
    redirect_uri: redirectUri,
    code: authorizationCode,
    code_verifier: initialCodeVerifier    
  };
  console.log(queryParams)
  const postBodyContent = Object.keys(queryParams).map(function (key) {
			return encodeURIComponent(key) + '=' + encodeURIComponent(queryParams[key]);
		}).join('&');
  console.log(postBodyContent)
  console.log(clientId)
  const secret = jsrsasign.stob64(`${clientId}:${initialCodeVerifier}`)
  console.log(secret)

  await fetch("https://eu-gb.appid.cloud.ibm.com/oauth/v4/3143b98e-2035-4fac-b765-879402a1b585/token", {
    method: "POST",
    headers: {
      'Authorization': 'Basic '+secret,
      'Content-Type': 'application/x-www-form-urlencoded'
    },
    body: postBodyContent
  }).then(response => {
      console.log(response)
      if (!response.ok) throw Error(response.statusText);
      return response.json();
    })
    .then(json => {
      // save the tokens
      store.state.tokens = json;
      console.log(store)
    }).catch(err => console.log(err))
}

function getRandomString(length)
{
  return jsrsasign.KJUR.crypto.Util.getRandomHexOfNbytes(length / 2);
}

async function fetchDataFromPrivateAPI() {
  console.log(store.state.tokens);
  const data = await fetch("https://31f5ff35.eu-gb.apigw.appdomain.cloud/private-authtest/Hello", {
    headers: {
      Authorization: `Bearer ${store.state.tokens.access_token}`,
      'Content-type': "application/json"
    }
  }).then((res) => {
    const data = res.json()
    console.log(data)
    return data
  });
  return data
}

export default {
  name: 'app',
  data () {
    return {
      msg: "hello world",
      private_message: null,
      authenticationUrl: null
    }
  },

  // notice the async can be applied to the event binding methods for Vue
  async created() {
    // When we return from our IBM App ID login page
    // we then can check the code and state
    const currentLocation = new URL(window.location.href);
    const hrefBase = currentLocation.href.substr(0,currentLocation.href.indexOf('?'));
    if ( hrefBase == redirectUri ) {
      console.log("REDIRECTED FROM APP ID")
      await getOAuthAccessToken();
      const data = await fetchDataFromPrivateAPI();
      this.private_message = data.message
    }
  },

  methods: {
    loginWithOAuth2() {
      // this function fires when user clicks a button 
      const state = getRandomString(20)
      const codeVerifier = getRandomString(44)
      console.log("State: "+state)
      console.log("CodeVerifier: "+codeVerifier)
      save_state_and_verifier(state, codeVerifier)
      generatePKCECodeChallenge(codeVerifier).then((challenge) => {
        console.log("Challenge: "+challenge);
        // whatever you generated, example:
        // M5oPBP6RmJPh5QsGtx6ptVH7S2yjqt8sum96jBCyhZg
        this.authenticationUrl = buildLoginUrl(state, challenge);

        window.location.href = this.authenticationUrl;
      });
    }
  }
  
}
</script>

<style>
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
}

h1, h2 {
  font-weight: normal;
}

ul {
  list-style-type: none;
  padding: 0;
}

li {
  display: inline-block;
  margin: 0 10px;
}

a {
  color: #42b983;
}
</style>
