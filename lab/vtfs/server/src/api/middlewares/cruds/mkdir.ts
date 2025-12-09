import {Context, Next} from "koa";
import {repository} from "@models/repository";

export async function mkdir(ctx: Context, next: Next) {
    const token = String(ctx.query.token);
    const parentIno = parseInt(ctx.query.parent_ino as string, 10) || 0;
    const name = ctx.query.name as string;
    const newDirectory = await repository.create(token, parentIno, true, null, name);
    ctx.body = { ino: newDirectory.ino };
    await next();
}