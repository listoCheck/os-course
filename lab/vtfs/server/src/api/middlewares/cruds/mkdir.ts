import {Context, Next} from "koa";
import {repository} from "@models/repository";

export async function mkdir(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const parentIno = parseInt(ctx.query.parent_ino as string, 10) || 0;
    const name = ctx.query.name as string;

    console.log("[mkdir] Incoming request:", { token, parentIno, name });

    const newDirectory = await repository.create(token, parentIno, true, null, name);

    console.log("[mkdir] Response:", { ino: newDirectory.ino });

    ctx.body = { ino: newDirectory.ino };
    await next();
}