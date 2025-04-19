// Import the functions you need from the SDKs you need
import { initializeApp } from "https://www.gstatic.com/firebasejs/9.16.0/firebase-app.js";
import {getDatabase} from 'https://www.gstatic.com/firebasejs/9.16.0/firebase-database.js';

//Dado EXTREMAMENTE SENSÍVEL!
const firebaseConfig = {
  apiKey: "",
  authDomain: "",
  databaseURL: "",
  projectId:"" ,
  storageBucket: "",
  messagingSenderId: "",
  appId: ""
};

// Inicializando a base de dados.
export const app = initializeApp(firebaseConfig);
export const db = getDatabase(app);
