console.log("✅ JavaScript chargé ! FileHandler marche.");

function testJS() {
    document.getElementById("js-result").innerHTML = "✅ JavaScript fonctionne ! FileHandler sert correctement les fichiers .js";
    document.getElementById("js-result").style.color = "#4CAF50";
    document.getElementById("js-result").style.fontWeight = "bold";
    alert("✅ JS OK !");
}

window.onload = function() {
    console.log("✅ Page chargée, tous les fichiers statiques ont été servis !");
};