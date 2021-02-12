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

function save_state_and_verifier(state, code_verifier) {
  // save the state and verifier ready for reload after sign in
  if ( window.location.search.includes("state") ) return;
  const storage = window.sessionStorage;
  storage.clear();
  storage.setItem("state", state);
  storage.setItem("code_verifier", code_verifier);
}

// required to bring in async/await babel functionality
import 'regenerator-runtime/runtime'
async function generateCodeChallenge(codeVerifier) {
  const encoder = new TextEncoder();
  const data = encoder.encode(codeVerifier);
  const digest = await window.crypto.subtle.digest("SHA-256", data);
  const base64Digest = base64encode(digest);
  // you can extract this replacing code to a function
  return base64Digest
    .replace(/\+/g, "-")
    .replace(/\//g, "_")
    .replace(/=/g, "");
}

// Proto-code. Don't use for production
function buildLoginUrl(challenge) {
  const link = document.querySelector("a");
  const linkValue = new URL(baseUrl);

  const queryParams = {
    client_id: clientId, 
    response_type: "code",
    state: state,
    code_challenge: challenge,
    code_challenge_method: "S256",
    redirect_uri: redirectUri
  };

  for (const param in queryParams) {
    linkValue.searchParams.append(param, queryParams[param]);
  }

  return linkValue
}

// here we build the authentication string that will send us off to IBM App ID
const state = randomstring.generate()
const code_verifier = randomstring.generate(128)

// this should be put in at build time and not be kept in repository
// TODO fix this 
const clientId = IBM_APP_ID_CLIENTID
const responseType = 'code'
const redirectUri='http://localhost:8080/auth'
const discoveryEndpoint = IBM_APP_ID_DISCOVERY_ENDPOINT
const baseUrl = "https://eu-gb.appid.cloud.ibm.com/oauth/v4/3143b98e-2035-4fac-b765-879402a1b585/authorization";
let authentication_url = null;

function getToken() {
  const tokenEndpoint = new URL(
    "https://eu-gb.appid.cloud.ibm.com/oauth/v4/3143b98e-2035-4fac-b765-879402a1b585/token"
  );

  const currentLocation = new URL(window.location.href);
  const authorizationCode = currentLocation.searchParams.get("code");
  const stateFromLocation = currentLocation.searchParams.get("state");
  const initialCodeVerifier = window.sessionStorage.getItem("code_verifier");

  // This is a good place for checking the state too  
  console.log(window.sessionStorage.getItem("state"))
  console.log(stateFromLocation)
  if (window.sessionStorage.getItem("state") !== stateFromLocation){
    throw Error("Probable session hijacking attack!");
  }

  const queryParams = {
    client_id: clientId,
    grant_type: "authorization_code",
    state: stateFromLocation,
    code: authorizationCode,
    code_verifier: initialCodeVerifier,
    redirect_uri: redirectUri
  };

  for (const param in queryParams) {
    tokenEndpoint.searchParams.append(param, queryParams[param]);
  }

  fetch(`${tokenEndpoint.origin}${tokenEndpoint.pathname}`, {
    method: "POST",
    body: tokenEndpoint.searchParams,
    headers: {
      "Content-Type": "application/x-www-form-urlencoded",
      Accept: "application/json"
    }
  }).then(response => {
      if (!response.ok) throw Error(response.statusText);
      return response.json();
    })
    .then(json => {
      // save the tokens
      store.state.tokens = json;
      console.log(store)
      fetchData();
    });
}

function fetchData() {
  console.log(store.state.tokens);
  fetch("https://31f5ff35.eu-gb.apigw.appdomain.cloud/private-authtest/Hello", {
    headers: {
      Authorization: `Bearer ${store.state.tokens.access_token}`
    }
  }).then((res) => {
    console.log(res);
  });
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
  methods: {
    loginWithOAuth2() {
      // check if we are coming back from our App ID login sequence
      const currentLocation = new URL(window.location.href);
      const href_base = currentLocation.href.substr(0,currentLocation.href.indexOf('?'));
      const stateFromLocation = currentLocation.searchParams.get("state");
      if (window.sessionStorage.getItem("state") !== stateFromLocation){
          throw Error("Probable session hijacking attack!");
      }
      if ( href_base == redirect_uri ) {
        console.log("REDIRECTED FROM APP ID")
        getToken();
      }
      

      save_state_and_verifier(state, code_verifier)
      generateCodeChallenge(code_verifier).then((challenge) => {
        console.log("Challenge: "+challenge);
        // whatever you generated, example:
        // M5oPBP6RmJPh5QsGtx6ptVH7S2yjqt8sum96jBCyhZg
        this.authenticationUrl = buildLoginUrl(challenge);
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
