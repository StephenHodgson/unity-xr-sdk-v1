function ready(fn) {
  if (document.attachEvent ? document.readyState === "complete" : document.readyState !== "loading"){
    fn();
  } else {
    document.addEventListener('DOMContentLoaded', fn);
  }
}

ready(function(){
    document.querySelectorAll(".toggler").forEach(function(toggler) {
        toggler.addEventListener('click', function(e) {
            e.preventDefault();
            var subRow = e.currentTarget.nextElementSibling;
            subRow.classList.toggle('toggler-hidden');
        });
    });
});
