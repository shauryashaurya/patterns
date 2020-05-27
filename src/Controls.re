open Types;
open Api;

let buttonsForShapes = (shapes, scene, setSelection, setScene, setColor) => {
  let positions = Hashtbl.create(10);
  switch (
    shapes->Belt.List.map(r => {
      (r, Calculate.resolveShape(scene, r, positions))
    })
  ) {
  | [] => []
  | [(s, _)] => [
      ("Black", (() => setColor(s, Some("#000")))),
      ("Fade", (() => setColor(s, None))),
    ]
  | [(s1, CCircle(l1)), (_, CCircle(l2))] => [
      (
        "Add points at intersections",
        (
          () => {
            let cross =
              Calculate.intersectCircles(l1.center, l1.r, l2.center, l2.r);
            let {sym} = scene.shapes->Belt.Map.String.getExn(s1.id);
            let (scene, sels) =
              cross->Belt.List.reduce(
                (scene, []),
                ((scene, sels), pos) => {
                  let (scene, id) = scene->Api.Point.abs(~sym, pos.x, pos.y);
                  (scene, [{id, index: 0}, ...sels]);
                },
              );
            setScene(scene);
            setSelection(Some(Points(sels)));
          }
        ),
      ),
    ]
  | [(s1, CLine(l1)), (_, CLine(l2))] => [
      (
        "Add point at intersection",
        (
          () => {
            let cross = Calculate.intersection(l1.p1, l1.p2, l2.p1, l2.p2);
            switch (cross) {
            | None => ()
            | Some(cross) =>
              let {sym} = scene.shapes->Belt.Map.String.getExn(s1.id);
              let (scene, id) = scene->Api.Point.abs(~sym, cross.x, cross.y);
              setScene(scene);
              setSelection(Some(Points([{id, index: 0}])));
            };
          }
        ),
      ),
    ]
  | _ => []
  };
};

let buttonsForPoints = (points, scene, setSelection, setScene) => {
  switch (points) {
  | [_] => []
  | [p1, p2] => [
      (
        "Add midpoint",
        (
          () => {
            let {pos: _, sym} = Belt.Map.String.getExn(scene.points, p1.id);
            // setSelection(None);
            let (scene, id) = scene->Api.Point.line(~sym, p1, p2, 0.5);
            setScene(scene);
            setSelection(Some(Points([{id, index: 0}])));
          }
        ),
      ),
      (
        "Add mirror point",
        (
          () => {
            let {pos: _, sym} = Belt.Map.String.getExn(scene.points, p1.id);
            // setSelection(None);
            let (scene, id) = scene->Api.Point.line(~sym, p2, p1, 2.);
            setScene(scene);
            setSelection(Some(Points([{id, index: 0}])));
          }
        ),
      ),
      (
        "Add circle",
        (
          () => {
            let {pos: _, sym} = Belt.Map.String.getExn(scene.points, p1.id);
            let (scene, id) = scene->Api.Shape.circle(~sym, p1, p2);
            setScene(scene);
            setSelection(Some(Shapes([{id, index: 0}])));
          }
        ),
      ),
      (
        "Add line",
        (
          () => {
            let {pos: _, sym} = Belt.Map.String.getExn(scene.points, p1.id);
            let (scene, id) = scene->Api.Shape.line(~sym, p1, p2);
            setScene(scene);
            setSelection(Some(Shapes([{id, index: 0}])));
          }
        ),
      ),
    ]
  | _ => []
  };
};

module Location = {
  type location;
  [@bs.val] external location: location = "location";
  [@bs.set] external setHash: (location, string) => unit = "hash";
  [@bs.get] external hash: location => string = "hash";
};

let force = x =>
  switch (x) {
  | None => failwith("unwrapped a None")
  | Some(x) => x
  };

let permalink = (scene: scene) => {
  Js.log(scene);
  Location.setHash(
    Location.location,
    Serialize.serializeAnyToJson(scene)->Js.Json.stringify,
  );
};

let getInitial = default => {
  let current =
    Location.hash(Location.location)->Js.Global.decodeURIComponent;
  if (String.length(current) > 1) {
    let data =
      Serialize.unserializeAnyFromJsonUnsafe(
        Js.String2.sliceToEnd(current, ~from=1)->Js.Json.parseExn,
      );
    data;
  } else {
    default;
  };
};

[@react.component]
let make =
    (
      ~selection,
      ~setSelection,
      ~scene,
      ~setScene,
      ~onUndo,
      ~togglePoints,
      ~setColor,
    ) => {
  let buttons = [
    ("Permalink", () => permalink(scene)),
    ("Clear scene", () => setScene(Api.init())),
    ("Toggle points", () => togglePoints()),
    ("Undo", onUndo),
    ...switch (selection) {
       | None => []
       | Some(selection) => [
           ("Clear", (() => setSelection(None))),
           ...switch (selection) {
              | Points(items) =>
                buttonsForPoints(items, scene, setSelection, setScene)
              | Shapes(shapes) =>
                buttonsForShapes(
                  shapes,
                  scene,
                  setSelection,
                  setScene,
                  setColor,
                )
              },
         ]
       },
  ];
  <div>
    {buttons
     ->Array.of_list
     ->Belt.Array.map(((title, action)) =>
         <button key=title onClick={_ => action()}>
           {React.string(title)}
         </button>
       )
     ->React.array}
  </div>;
};